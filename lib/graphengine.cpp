#include "graphengine.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>

#include <limits>
#include <queue>
#include <vector>
#include <utility>

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

GraphEngine::GraphEngine() = default;

// String → NodeId translation — the ONLY place strings are used internally.
GraphEngine::NodeId GraphEngine::resolve(const QString &label) const
{
    return m_labelToId.value(label, kInvalid);
}

// ─────────────────────────────────────────────────────────────────────────────
// Graph construction
// ─────────────────────────────────────────────────────────────────────────────

void GraphEngine::addNode(const QString &name)
{
    if (name.isEmpty() || m_labelToId.contains(name))
        return;

    const NodeId id = static_cast<NodeId>(m_nodes.size());
    m_labelToId.insert(name, id);
    m_nodes.append(NodeData{name, false});
    m_adj.emplace_back();       // empty adjacency list for this node
}

void GraphEngine::addEdge(const QString &from, const QString &to, double cost)
{
    if (from.isEmpty() || to.isEmpty()) return;

    addNode(from);
    addNode(to);

    const NodeId f = resolve(from);
    const NodeId t = resolve(to);

    // Check for duplicate in the adjacency list of f (int comparisons only)
    for (const Link &lnk : std::as_const(m_adj[f])) {
        if (lnk.to == t) {
            qWarning() << "GraphEngine::addEdge: duplicate edge"
                       << from << "->" << to << "— skipped.";
            return;
        }
    }

    m_adj[f].append(Link{t, cost});
    ++m_edgeCount;
}

// ─────────────────────────────────────────────────────────────────────────────
// Runtime blocking
// ─────────────────────────────────────────────────────────────────────────────

void GraphEngine::blockNode(const QString &name, bool blocked)
{
    const NodeId id = resolve(name);
    if (id == kInvalid) { qWarning() << "GraphEngine::blockNode: unknown node" << name; return; }
    m_nodes[id].blocked = blocked;
}

void GraphEngine::unblockNode(const QString &name) { blockNode(name, false); }

bool GraphEngine::isBlocked(const QString &name) const
{
    const NodeId id = resolve(name);
    return id != kInvalid && m_nodes[id].blocked;
}

void GraphEngine::clearBlocked()
{
    for (NodeData &n : m_nodes) n.blocked = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON loading
// ─────────────────────────────────────────────────────────────────────────────

bool GraphEngine::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "GraphEngine::loadFromFile: cannot open" << filePath;
        return false;
    }
    return loadFromJson(file.readAll());
}

bool GraphEngine::loadFromJson(const QByteArray &json)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "GraphEngine::loadFromJson:" << err.errorString();
        return false;
    }

    const QJsonObject root = doc.object();

    // Map internal HTML IDs ("n1", "n2"…) → labels ("A", "B"…).
    // This temporary map is QHash<QString,QString> but it is only used
    // during loading, never in hot paths.
    QHash<QString, QString> idToLabel;

    for (const QJsonValue &nv : root["nodes"].toArray()) {
        const QJsonObject obj  = nv.toObject();
        const QString     id   = obj["id"].toString();
        const QString     lbl  = obj["label"].toString();
        if (id.isEmpty() || lbl.isEmpty()) continue;
        idToLabel.insert(id, lbl);
        addNode(lbl);
    }

    for (const QJsonValue &ev : root["edges"].toArray()) {
        const QJsonObject obj  = ev.toObject();
        const QString fromLbl  = idToLabel.value(obj["from"].toString());
        const QString toLbl    = idToLabel.value(obj["to"].toString());
        const double  cost     = obj["cost"].toDouble(1.0);
        if (fromLbl.isEmpty() || toLbl.isEmpty()) continue;
        addEdge(fromLbl, toLbl, cost);
    }

    qInfo() << "GraphEngine: loaded" << nodeCount() << "nodes and"
            << edgeCount() << "edges.";
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// XML import / export
//
// Format mirrors the JSON format so the same grafo.xml can be loaded by both
// the HTML editor and the C++ lib.
//
// <graphengine version="1">
//   <nodes>
//     <node id="n0" x="0" y="0" label="A"/>
//   </nodes>
//   <edges>
//     <edge id="e0" from="n0" to="n1" cost="1"/>
//   </edges>
// </graphengine>
//
// Node IDs follow the internal NodeId (0-based) so the id/from/to references
// are stable within a single file. Strings appear only at the boundary pass.
// ─────────────────────────────────────────────────────────────────────────────

bool GraphEngine::loadFromXmlFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "GraphEngine::loadFromXmlFile: cannot open" << filePath;
        return false;
    }
    return loadFromXml(file.readAll());
}

bool GraphEngine::loadFromXml(const QByteArray &xml)
{
    QXmlStreamReader reader(xml);
    QHash<QString, QString> idToLabel;  // xmlId -> label (boundary only)

    bool inNodes = false, inEdges = false;

    while (!reader.atEnd() && !reader.hasError()) {
        reader.readNext();

        if (reader.isEndElement()) {
            const auto n = reader.name();
            if      (n == QLatin1String("nodes")) inNodes = false;
            else if (n == QLatin1String("edges")) inEdges = false;
            continue;
        }

        if (!reader.isStartElement()) continue;

        const auto name = reader.name();
        if      (name == QLatin1String("nodes")) { inNodes = true;  inEdges = false; }
        else if (name == QLatin1String("edges")) { inEdges = true;  inNodes = false; }
        else if (name == QLatin1String("node") && inNodes) {
            const auto   attrs = reader.attributes();
            const QString id   = attrs.value(QLatin1String("id")).toString();
            const QString lbl  = attrs.value(QLatin1String("label")).toString();
            if (!id.isEmpty() && !lbl.isEmpty()) {
                idToLabel.insert(id, lbl);
                addNode(lbl);
            }
        }
        else if (name == QLatin1String("edge") && inEdges) {
            const auto    attrs    = reader.attributes();
            const QString fromId   = attrs.value(QLatin1String("from")).toString();
            const QString toId     = attrs.value(QLatin1String("to")).toString();
            const QString costStr  = attrs.value(QLatin1String("cost")).toString();
            const double  cost     = costStr.isEmpty() ? 1.0 : costStr.toDouble();
            const QString fromLbl  = idToLabel.value(fromId);
            const QString toLbl    = idToLabel.value(toId);
            if (!fromLbl.isEmpty() && !toLbl.isEmpty())
                addEdge(fromLbl, toLbl, cost);
        }
    }

    if (reader.hasError()) {
        qWarning() << "GraphEngine::loadFromXml:" << reader.errorString();
        return false;
    }

    qInfo() << "GraphEngine: loaded" << nodeCount() << "nodes and"
            << edgeCount() << "edges from XML.";
    return nodeCount() > 0;
}

QByteArray GraphEngine::toXml() const
{
    QByteArray out;
    QXmlStreamWriter xml(&out);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    xml.writeStartElement(QStringLiteral("graphengine"));
    xml.writeAttribute(QStringLiteral("version"), QStringLiteral("1"));

    // ── Nodes — NodeId is the stable id inside this file ─────────────────────
    xml.writeStartElement(QStringLiteral("nodes"));
    for (NodeId id = 0; id < static_cast<NodeId>(m_nodes.size()); ++id) {
        xml.writeEmptyElement(QStringLiteral("node"));
        xml.writeAttribute(QStringLiteral("id"),    QString("n%1").arg(id));
        xml.writeAttribute(QStringLiteral("x"),     QStringLiteral("0"));
        xml.writeAttribute(QStringLiteral("y"),     QStringLiteral("0"));
        xml.writeAttribute(QStringLiteral("label"), m_nodes[id].label);
    }
    xml.writeEndElement(); // nodes

    // ── Edges — iterate adjacency list ────────────────────────────────────────
    xml.writeStartElement(QStringLiteral("edges"));
    int eIdx = 0;
    for (NodeId u = 0; u < static_cast<NodeId>(m_nodes.size()); ++u) {
        for (const Link &lnk : m_adj[u]) {
            xml.writeEmptyElement(QStringLiteral("edge"));
            xml.writeAttribute(QStringLiteral("id"),   QString("e%1").arg(eIdx++));
            xml.writeAttribute(QStringLiteral("from"), QString("n%1").arg(u));
            xml.writeAttribute(QStringLiteral("to"),   QString("n%1").arg(lnk.to));
            xml.writeAttribute(QStringLiteral("cost"), QString::number(lnk.cost));
        }
    }
    xml.writeEndElement(); // edges

    xml.writeEndElement(); // graphengine
    xml.writeEndDocument();
    return out;
}

bool GraphEngine::saveToXmlFile(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "GraphEngine::saveToXmlFile: cannot open" << filePath;
        return false;
    }
    file.write(toXml());
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Pathfinding — Dijkstra
//
// All operations inside the loop use NodeId (int):
//   - dist[]  is std::vector<double>  — O(1) indexed, cache-friendly
//   - prev[]  is std::vector<NodeId>  — O(1) indexed, cache-friendly
//   - m_adj[u] iterates only over u's neighbors (adjacency list)
//   - m_nodes[id].blocked  — O(1) indexed bool check
//
// Strings are resolved once before the loop and rebuilt once after.
// ─────────────────────────────────────────────────────────────────────────────

GraphEngine::PathResult GraphEngine::findPath(const QString &from,
                                              const QString &to) const
{
    const PathResult kFail;

    // ── Boundary: QString → NodeId (the only string ops in this function) ───
    const NodeId srcId = resolve(from);
    const NodeId dstId = resolve(to);

    if (srcId == kInvalid || dstId == kInvalid) {
        qWarning() << "GraphEngine::findPath: unknown node(s)" << from << to;
        return kFail;
    }
    if (m_nodes[srcId].blocked || m_nodes[dstId].blocked)
        return kFail;
    if (srcId == dstId)
        return PathResult{true, {from}, 0.0, 0};

    // ── Dijkstra — everything below is int / double ──────────────────────────
    constexpr double kInf = std::numeric_limits<double>::infinity();
    const int N = m_nodes.size();

    std::vector<double> dist(N, kInf);
    std::vector<NodeId> prev(N, kInvalid);

    dist[srcId] = 0.0;

    // min-heap: (accumulated_cost, NodeId)
    using Entry = std::pair<double, NodeId>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pq;
    pq.push({0.0, srcId});

    while (!pq.empty()) {
        const auto [d, u] = pq.top();
        pq.pop();

        if (u == dstId) break;
        if (d > dist[u])  continue; // stale entry

        for (const Link &lnk : m_adj[u]) {
            if (m_nodes[lnk.to].blocked) continue;

            const double alt = dist[u] + lnk.cost;
            if (alt < dist[lnk.to]) {
                dist[lnk.to] = alt;
                prev[lnk.to] = u;
                pq.push({alt, lnk.to});
            }
        }
    }

    if (dist[dstId] == kInf) return kFail;

    // ── Reconstruct path — NodeId → label (strings only appear here) ─────────
    QStringList path;
    for (NodeId cur = dstId; cur != kInvalid; cur = prev[cur])
        path.prepend(m_nodes[cur].label);

    if (path.isEmpty() || path.first() != from) return kFail;

    return PathResult{true, path, dist[dstId], static_cast<int>(path.size()) - 1};
}

// ─────────────────────────────────────────────────────────────────────────────
// Graph info
// ─────────────────────────────────────────────────────────────────────────────

QStringList GraphEngine::nodeNames() const
{
    QStringList names;
    names.reserve(m_nodes.size());
    for (const NodeData &n : m_nodes) names.append(n.label);
    return names;
}

int GraphEngine::nodeCount() const { return m_nodes.size(); }
int GraphEngine::edgeCount() const { return m_edgeCount; }

void GraphEngine::clear()
{
    m_labelToId.clear();
    m_nodes.clear();
    m_adj.clear();
    m_edgeCount = 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Flow analysis - BFS flood-fill
//
// Mirrors the JavaScript floodFill() in index.html exactly:
//   1. BFS from emitter following directed edges (adjacency list = O(neighbors))
//   2. On hitting a blocked node: record it in blockedReached + record the
//      incoming edge — BFS does NOT continue past it (valve closed)
//   3. After BFS, mark edges as flowing only when dist[from] < dist[to]
//      (flow never goes sideways or backwards)
//   4. Edges to blocked nodes are always marked as flowing (flow reached valve)
//
// All inner loop work is int-indexed. Strings appear only at boundaries.
// ─────────────────────────────────────────────────────────────────────────────

GraphEngine::FloodResult GraphEngine::floodFill(const QString &emitterLabel) const
{
    FloodResult result;
    result.emitter = emitterLabel;

    const NodeId emitterId = resolve(emitterLabel);
    if (emitterId == kInvalid) {
        qWarning() << "GraphEngine::floodFill: unknown emitter" << emitterLabel;
        return result;
    }

    result.valid = true;

    // Emitter itself is blocked: no flow at all
    if (m_nodes[emitterId].blocked) {
        for (NodeId id = 0; id < static_cast<NodeId>(m_nodes.size()); ++id)
            if (id != emitterId && !m_nodes[id].blocked)
                result.unreachable.append(m_nodes[id].label);
        return result;
    }

    const int N = m_nodes.size();

    // dist[id] = BFS distance from emitter, -1 = unvisited
    std::vector<int>  dist(N, -1);
    std::vector<bool> isBlockedHit(N, false);

    dist[emitterId] = 0;

    struct PendingEdge { NodeId from; NodeId to; double cost; };
    std::vector<PendingEdge> edgesToBlocked;

    std::queue<NodeId> q;
    q.push(emitterId);

    // ── BFS — pure int, no strings ────────────────────────────────────────────
    while (!q.empty()) {
        const NodeId u = q.front(); q.pop();

        for (const Link &lnk : m_adj[u]) {
            if (m_nodes[lnk.to].blocked) {
                isBlockedHit[lnk.to] = true;          // idempotent flag
                edgesToBlocked.push_back({u, lnk.to, lnk.cost});
                continue;
            }
            if (dist[lnk.to] != -1) continue;
            dist[lnk.to] = dist[u] + 1;
            q.push(lnk.to);
        }
    }

    // ── Boundary: NodeId → label (all string work happens here, once) ─────────
    for (NodeId id = 0; id < N; ++id) {
        if (id == emitterId) continue;
        if (isBlockedHit[id])
            result.blockedReached.append(m_nodes[id].label);
        else if (dist[id] != -1)
            result.reachable.append(m_nodes[id].label);
        else
            result.unreachable.append(m_nodes[id].label);
    }

    for (NodeId u = 0; u < N; ++u) {
        if (dist[u] == -1) continue;
        for (const Link &lnk : m_adj[u]) {
            if (dist[lnk.to] != -1 && dist[u] < dist[lnk.to])
                result.flowEdges.append({m_nodes[u].label, m_nodes[lnk.to].label, lnk.cost});
        }
    }
    for (const PendingEdge &pe : edgesToBlocked)
        result.flowEdges.append({m_nodes[pe.from].label, m_nodes[pe.to].label, pe.cost});

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// PathResult / FloodResult — toString
// ─────────────────────────────────────────────────────────────────────────────

QString GraphEngine::PathResult::toString() const
{
    if (!found) return QStringLiteral("Sem caminho.");
    return QStringLiteral("[ %1 ]  custo: %2  |  %3 aresta(s)")
        .arg(path.join(QStringLiteral(" -> ")))
        .arg(totalCost)
        .arg(steps);
}

QString GraphEngine::FloodResult::FlowEdge::toString() const
{
    return QStringLiteral("%1 -> %2  (custo: %3)").arg(from, to).arg(cost);
}

QString GraphEngine::FloodResult::toString() const
{
    if (!valid) return QStringLiteral("FloodResult invalido.");

    QStringList lines;
    lines << QStringLiteral("Emissor      : %1").arg(emitter);
    lines << QStringLiteral("Alcancaveis  : %1").arg(
                reachable.isEmpty() ? QStringLiteral("(nenhum)") : reachable.join(", "));
    lines << QStringLiteral("Valvulas hit : %1").arg(
                blockedReached.isEmpty() ? QStringLiteral("(nenhuma)") : blockedReached.join(", "));
    lines << QStringLiteral("Inalcancaveis: %1").arg(
                unreachable.isEmpty() ? QStringLiteral("(nenhum)") : unreachable.join(", "));
    lines << QStringLiteral("Arestas fluxo: %1").arg(flowEdges.size());
    for (const FlowEdge &e : flowEdges)
        lines << QStringLiteral("  ") + e.toString();
    return lines.join(QStringLiteral("\n"));
}
