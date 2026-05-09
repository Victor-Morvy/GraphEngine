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

    // ── Carregar JSON ─────────────────────────────────────────────────────────
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

    // ── Dijkstra ──────────────────────────────────────────────────────────────
    section("Dijkstra: ENGINE_2 -> PACK2  (sem bloqueios)");
    out << "  " << g.findPath("ENGINE_2", "PACK2").toString() << "\n";

    section("Dijkstra: ENGINE_2 -> PACK2  (Xbleed bloqueado)");
    g.blockNode("Xbleed");
    out << "  " << g.findPath("ENGINE_2", "PACK2").toString() << "\n";
    g.clearBlocked();

    // ── FloodFill ─────────────────────────────────────────────────────────────
    section("FloodFill — emissor: ENGINE_1  (sem bloqueios)");
    printFlood(g.floodFill("ENGINE_1"));

    section("FloodFill — emissor: ENGINE_1  (SOV_PACK1 = valvula fechada)");
    g.blockNode("SOV_PACK1");
    printFlood(g.floodFill("ENGINE_1"));
    g.clearBlocked();

    section("FloodFill — emissor: APU  (propaga pela rede inteira)");
    printFlood(g.floodFill("APU"));

    // ── XML: export → import → verificar roundtrip ───────────────────────────
    section("XML Export: salvar grafo como XML");
    const QString xmlPath = QStringLiteral("C:/Users/Victor/Documents/Repositories/GraphEngine/grafo.xml");
    if (g.saveToXmlFile(xmlPath))
        out << "  Salvo em: " << xmlPath << "\n";
    else
        out << "  [ERRO] nao foi possivel salvar XML.\n";

    section("XML Import: carregar grafo.xml em novo engine");
    GraphEngine gXml;
    if (gXml.loadFromXmlFile(xmlPath)) {
        out << "  Nos    : " << gXml.nodeCount() << "\n";
        out << "  Arestas: " << gXml.edgeCount() << "\n";
        out << "  Nomes  : " << gXml.nodeNames().join(", ") << "\n";

        const bool nosOk    = gXml.nodeCount() == g.nodeCount();
        const bool arestasOk = gXml.edgeCount() == g.edgeCount();
        out << "  Roundtrip nos    : " << (nosOk     ? "OK" : "FALHOU") << "\n";
        out << "  Roundtrip arestas: " << (arestasOk ? "OK" : "FALHOU") << "\n";
    } else {
        out << "  [ERRO] nao foi possivel carregar XML.\n";
    }

    section("FloodFill no grafo carregado do XML — emissor: ENGINE_1");
    printFlood(gXml.floodFill("ENGINE_1"));

    section("Dijkstra no grafo carregado do XML — ENGINE_2 -> PACK2");
    out << "  " << gXml.findPath("ENGINE_2", "PACK2").toString() << "\n";

    out << "\n";
    return 0;
}
