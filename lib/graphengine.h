#pragma once

#include <QString>
#include <QStringList>
#include <QHash>
#include <QVector>

/**
 * GraphEngine - directed weighted graph with Dijkstra pathfinding
 *               and BFS flood-fill flow analysis.
 *
 * Public API uses QString for ergonomics.
 * Internally every node is an int (NodeId); strings only touch the graph
 * at the public boundary (one hash lookup per call, never inside hot loops).
 *
 * Loads graphs exported by the GraphEngine HTML editor (grafo.json).
 */
class GraphEngine
{
public:
    using NodeId = int;

    // -------------------------------------------------------------------------
    // PathResult - returned by findPath()
    // -------------------------------------------------------------------------
    struct PathResult {
        bool        found     = false;
        QStringList path;         // node labels in traversal order
        double      totalCost = 0.0;
        int         steps     = 0; // number of edges traversed

        QString toString() const;
    };

    // -------------------------------------------------------------------------
    // FloodResult - returned by floodFill()
    //
    // Mirrors exactly what the HTML editor shows in "Analise de Fluxo":
    //   reachable      - nodes the flow reached (excluding the emitter itself)
    //   blockedReached - blocked nodes the flow HIT but could not cross (valves)
    //   unreachable    - nodes the flow never reached (not blocked, just cut off)
    //   flowEdges      - directed edges that carry flow
    //
    // Flow rules (same as the editor):
    //   - Follows edge direction only (no uphill / reverse flow).
    //   - Stops at blocked nodes: the incoming edge is marked as flowing
    //     (flow arrived at the valve), but BFS does not continue past them.
    //   - An edge u->v carries flow only when dist(u) < dist(v) in BFS,
    //     so reverse edges (B->A when flow came A->B) are never marked.
    // -------------------------------------------------------------------------
    struct FloodResult {
        bool    valid = false;

        QString     emitter;
        QStringList reachable;       // labels of freely-reached nodes (excl. emitter)
        QStringList blockedReached;  // labels of valves the flow hit
        QStringList unreachable;     // labels of nodes the flow never touched

        struct FlowEdge {
            QString from, to;
            double  cost = 1.0;
            QString toString() const;
        };
        QList<FlowEdge> flowEdges;   // edges carrying flow (incl. edges to valves)

        QString toString() const;
    };

    GraphEngine();

    // -------------------------------------------------------------------------
    // Graph construction
    // -------------------------------------------------------------------------

    /** Add a node by label. No-op if the label already exists. */
    void addNode(const QString &name);

    /**
     * Add a directed edge from -> to with the given cost (default 1).
     * For bidirectional behaviour call addEdge(a,b) and addEdge(b,a).
     * Auto-creates missing nodes. Warns and skips duplicate edges.
     */
    void addEdge(const QString &from, const QString &to, double cost = 1.0);

    // -------------------------------------------------------------------------
    // Runtime blocking
    // -------------------------------------------------------------------------

    void blockNode  (const QString &name, bool blocked = true);
    void unblockNode(const QString &name);
    bool isBlocked  (const QString &name) const;
    void clearBlocked();

    // -------------------------------------------------------------------------
    // JSON / XML loading and export (GraphEngine HTML editor format)
    // -------------------------------------------------------------------------

    bool loadFromFile   (const QString &filePath);
    bool loadFromJson   (const QByteArray &json);

    bool loadFromXmlFile(const QString &filePath);
    bool loadFromXml    (const QByteArray &xml);
    QByteArray toXml    () const;
    bool saveToXmlFile  (const QString &filePath) const;

    // -------------------------------------------------------------------------
    // Pathfinding - Dijkstra
    // -------------------------------------------------------------------------

    PathResult findPath(const QString &from, const QString &to) const;

    // -------------------------------------------------------------------------
    // Flow analysis - BFS flood-fill
    // -------------------------------------------------------------------------

    FloodResult floodFill(const QString &emitter) const;

    // -------------------------------------------------------------------------
    // Graph info
    // -------------------------------------------------------------------------

    QStringList nodeNames() const;
    int         nodeCount() const;
    int         edgeCount() const;
    void        clear();

private:
    static constexpr NodeId kInvalid = -1;

    struct NodeData {
        QString label;
        bool    blocked = false;
    };

    // Half-edge in the adjacency list ("from" is implicit = index in m_adj).
    struct Link {
        NodeId to;
        double cost;
    };

    NodeId resolve(const QString &label) const;

    QHash<QString, NodeId>   m_labelToId; // label -> id  (boundary only)
    QVector<NodeData>        m_nodes;     // m_nodes[id]
    QVector<QVector<Link>>   m_adj;       // m_adj[fromId] -> neighbor list
    int                      m_edgeCount = 0;
};
