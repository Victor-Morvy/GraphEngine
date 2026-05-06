#pragma once

#include <QString>
#include <QStringList>
#include <QHash>
#include <QVector>

/**
 * GraphEngine — directed weighted graph with Dijkstra pathfinding.
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

    // ── Result returned by findPath ──────────────────────────────────────────
    struct PathResult {
        bool        found     = false;
        QStringList path;     // node labels in order
        double      totalCost = 0.0;
        int         steps     = 0; // number of edges traversed

        QString toString() const;
    };

    GraphEngine();

    // ── Graph construction ───────────────────────────────────────────────────

    /** Add a node by label. No-op if the label already exists. */
    void addNode(const QString &name);

    /**
     * Add a directed edge from → to with the given cost (default 1).
     * For bidirectional behaviour call addEdge(a,b) and addEdge(b,a).
     * Auto-creates missing nodes. Warns and skips duplicate edges.
     */
    void addEdge(const QString &from, const QString &to, double cost = 1.0);

    // ── Runtime blocking ─────────────────────────────────────────────────────

    void blockNode  (const QString &name, bool blocked = true);
    void unblockNode(const QString &name);
    bool isBlocked  (const QString &name) const;
    void clearBlocked();

    // ── JSON loading (GraphEngine HTML editor format) ────────────────────────

    bool loadFromFile(const QString &filePath);
    bool loadFromJson(const QByteArray  &json);

    // ── Pathfinding (Dijkstra) ───────────────────────────────────────────────

    PathResult findPath(const QString &from, const QString &to) const;

    // ── Graph info ───────────────────────────────────────────────────────────

    QStringList nodeNames() const;
    int         nodeCount() const;
    int         edgeCount() const;
    void        clear();

private:
    static constexpr NodeId kInvalid = -1;

    // One entry per node, indexed by NodeId (a plain int).
    struct NodeData {
        QString label;
        bool    blocked = false;
    };

    // One half-edge in the adjacency list.
    // "from" is implicit (it is the index in m_adj).
    struct Link {
        NodeId to;
        double cost;
    };

    // ── String→id translation (used only at the public boundary) ────────────
    NodeId resolve(const QString &label) const;

    // ── Internal storage (all hot-path access is int-indexed) ───────────────
    QHash<QString, NodeId>     m_labelToId; // label → id  (boundary only)
    QVector<NodeData>          m_nodes;     // m_nodes[id]
    QVector<QVector<Link>>     m_adj;       // m_adj[fromId] → neighbor list
    int                        m_edgeCount = 0;
};
