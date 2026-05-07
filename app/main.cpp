#include <QCoreApplication>
#include <QTextStream>

#include "graphengine.h"

static QTextStream out(stdout);

static void section(const QString &title)
{
    out << "\n────────────────────────────────────────────────────────\n";
    out << "  " << title << "\n";
    out << "────────────────────────────────────────────────────────\n";
}

static void printFlood(const GraphEngine::FloodResult &r)
{
    if (!r.valid) { out << "  [invalido]\n"; return; }
    out << "  Emissor       : " << r.emitter << "\n";
    out << "  Alcancaveis   : "
        << (r.reachable.isEmpty() ? QStringLiteral("(nenhum)") : r.reachable.join(", ")) << "\n";
    out << "  Valvulas hit  : "
        << (r.blockedReached.isEmpty() ? QStringLiteral("(nenhuma)") : r.blockedReached.join(", ")) << "\n";
    out << "  Inalcancaveis : "
        << (r.unreachable.isEmpty() ? QStringLiteral("(nenhum)") : r.unreachable.join(", ")) << "\n";
    out << "  Arestas fluxo : " << r.flowEdges.size() << "\n";
    for (const GraphEngine::FloodResult::FlowEdge &e : r.flowEdges)
        out << "    " << e.toString() << "\n";
    out.flush();
}

// Grafo embutido usado quando grafo.json nao existe.
//
//   Entrada -1-> Sala -2-> Quarto -1-> Garagem
//      |                               ^
//      +-1-> Cozinha -2-> Banheiro -1--+
//
static void buildFallbackGraph(GraphEngine &g)
{
    g.addNode("Entrada");  g.addNode("Sala");
    g.addNode("Cozinha");  g.addNode("Quarto");
    g.addNode("Banheiro"); g.addNode("Garagem");

    g.addEdge("Entrada",  "Sala",     1.0);
    g.addEdge("Entrada",  "Cozinha",  1.0);
    g.addEdge("Sala",     "Quarto",   2.0);
    g.addEdge("Cozinha",  "Banheiro", 2.0);
    g.addEdge("Quarto",   "Garagem",  1.0);
    g.addEdge("Banheiro", "Garagem",  1.0);
    g.addEdge("Sala",     "Banheiro", 3.0);
    g.addEdge("Cozinha",  "Quarto",   3.0);
}

// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    GraphEngine g;
    const QString jsonPath = (argc > 1)
        ? QString::fromLocal8Bit(argv[1])
        : QStringLiteral("C:/Users/Victor/Documents/Repositories/GraphEngine/grafo.json");

    if (g.loadFromFile(jsonPath)) {
        out << "\nGrafo carregado de: " << jsonPath << "\n";
    } else {
        out << "\n[AVISO] '" << jsonPath << "' nao encontrado — usando grafo embutido.\n";
        buildFallbackGraph(g);
    }

    out << "  Nos    : " << g.nodeCount() << "\n";
    out << "  Arestas: " << g.edgeCount() << "\n";
    out << "  Nomes  : " << g.nodeNames().join(", ") << "\n";

    // ── Dijkstra: ENGINE_2 → PACK2 ───────────────────────────────────────────
    section("Dijkstra: ENGINE_2 -> PACK2  (sem bloqueios)");
    out << "  " << g.findPath("ENGINE_2", "PACK2").toString() << "\n";

    section("Dijkstra: ENGINE_2 -> PACK2  (Xbleed bloqueado)");
    g.blockNode("Xbleed");
    out << "  " << g.findPath("ENGINE_2", "PACK2").toString() << "\n";
    g.clearBlocked();

    // ── FloodFill 1: ENGINE_1 sem bloqueios ──────────────────────────────────
    section("FloodFill 1 — emissor: ENGINE_1  (sem bloqueios)");
    printFlood(g.floodFill("ENGINE_1"));

    // ── FloodFill 2: ENGINE_1 com SOV_PACK1 bloqueado ────────────────────────
    section("FloodFill 2 — emissor: ENGINE_1  (SOV_PACK1 = valvula fechada)");
    g.blockNode("SOV_PACK1");
    printFlood(g.floodFill("ENGINE_1"));
    g.clearBlocked();

    // ── FloodFill 3: Xbleed com si proprio bloqueado ─────────────────────────
    section("FloodFill 3 — emissor: Xbleed  (Xbleed bloqueado = sem fluxo)");
    g.blockNode("Xbleed");
    printFlood(g.floodFill("Xbleed"));
    g.clearBlocked();

    // ── FloodFill 4: APU (propaga por toda a rede) ────────────────────────────
    section("FloodFill 4 — emissor: APU  (propaga pela rede inteira)");
    printFlood(g.floodFill("APU"));

    // ── FloodFill 5: APU com Xbleed bloqueado (divide a rede) ────────────────
    section("FloodFill 5 — emissor: APU  (Xbleed bloqueado)");
    g.blockNode("Xbleed");
    printFlood(g.floodFill("APU"));
    g.clearBlocked();

    out << "\n";
    return 0;
}
