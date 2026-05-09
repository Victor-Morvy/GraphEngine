# GraphEngine

Editor visual de grafos direcionados com pesos, exportação JSON/XML e biblioteca C++ com Dijkstra e BFS flood-fill.

O projeto tem dois componentes independentes que se comunicam via JSON ou XML:

| Componente | Tecnologia | Função |
|---|---|---|
| `index.html` | HTML5 / Canvas / JS | Editor visual — cria, edita, analisa e exporta grafos |
| `lib/` | C++ / Qt | Biblioteca estática — pathfinding (Dijkstra) e análise de fluxo (BFS flood-fill) |

---

## Editor HTML (`index.html`)

Abra diretamente no navegador — sem servidor, sem dependências.

### Modos de edição

| Atalho | Modo | Ação |
|---|---|---|
| `S` | Selecionar | Clica/arrasta nós; clica arestas para selecionar |
| `N` | Adicionar Nó | Clique no canvas cria um nó com label automático |
| `E` | Adicionar Aresta | Clique em dois nós; popup define direção e custo |
| `D` / `Del` | Deletar | Clique no elemento para remover |
| `Esc` | — | Cancela ação atual |

- **Duplo-clique** em nó → renomear  
- **Duplo-clique** em aresta → editar custo  
- **Botão direito** → menu de contexto  
- **Arrastar fundo** ou **scroll** → navegar no canvas  

### Arestas

Ao conectar dois nós aparece um popup com três opções:

```
→ A → B          (aresta simples, A para B)
← B → A          (aresta simples, B para A)
↔ Bidirecional   (cria as duas arestas)
```

Cada aresta tem um **custo** (padrão `1`).

### Modo Play ▶

Ativa o modo de teste em runtime:

- Clique nos nós para **bloqueá-los / desbloqueá-los** (válvulas fechadas)
- Nós bloqueados são ignorados no pathfinding e na análise de fluxo
- Painel lateral mostra resultado: caminho, custo total e número de arestas

Algoritmos disponíveis: **Dijkstra** (custo mínimo) e **BFS** (menos saltos).

### Análise de Fluxo ⬡

Simula líquido escoando a partir de um **nó emissor** pelo grafo direcionado:

| Visual | Significado |
|---|---|
| Âmbar pulsante + badge `EMIT` | Nó emissor (fonte do fluxo) |
| Ciano + glow | Nó alcançável pelo fluxo |
| Dashes ciano animados | Aresta carregando fluxo (animação no sentido da seta) |
| Vermelho + badge `BLOQ` | Válvula fechada — fluxo para antes deste nó |
| 20% opacidade | Elemento inalcançável |

**Regras do fluxo:**
- Segue apenas o sentido das setas (grafo direcionado)
- Para em nós bloqueados (válvulas fechadas)
- Uma aresta só é marcada como fluindo se `dist(from) < dist(to)` — o fluxo nunca volta por onde já passou, mesmo que exista aresta reversa
- Re-analisa automaticamente ao bloquear/desbloquear nós no modo Play

### Persistência

- **Auto-save** em `localStorage` a cada ação — carrega automaticamente ao reabrir
- **Exportar** → baixa `grafo.json`
- **Carregar** → importa um `grafo.json` existente
- **Exportar XML** → baixa `grafo.xml` (mesmo esquema do JSON, formato XML)
- **Carregar XML** → importa um `grafo.xml`; nós com `x=0/y=0` são auto-distribuídos em linha

---

## Biblioteca C++ (`lib/`)

Requer **Qt 6** e **Qt Creator** com MSVC 2019+ ou MinGW.

### Compilar

```
# Abra GraphEngine.pro no Qt Creator e clique em Build.
# A lib compila primeiro (dependência declarada no subdirs .pro).
```

Ou via linha de comando:

```bash
cd build
qmake ../GraphEngine.pro
make          # ou nmake / jom no Windows
```

O `grafo.json` é copiado automaticamente ao lado do executável após cada build.

### API

```cpp
#include "graphengine.h"

GraphEngine g;

// ── Carregar do JSON exportado pelo editor ──────────────────────────────────
g.loadFromFile("grafo.json");

// ── Ou construir manualmente ────────────────────────────────────────────────
g.addNode("A");
g.addNode("B");
g.addEdge("A", "B", 2.5);   // A → B, custo 2.5
g.addEdge("B", "A", 1.0);   // bidirecional: B → A, custo 1.0

// ── Bloquear nós em runtime ─────────────────────────────────────────────────
g.blockNode("B");            // B será ignorado no pathfinding
g.isBlocked("B");            // true
g.clearBlocked();            // desbloqueia todos

// ── Pathfinding — Dijkstra ──────────────────────────────────────────────────
GraphEngine::PathResult r = g.findPath("A", "C");

r.found      // bool
r.path       // QStringList{"A", "B", "C"}
r.totalCost  // double — soma dos custos das arestas
r.steps      // int    — número de arestas percorridas
r.toString() // QString formatada para exibição

// ── Análise de fluxo — BFS flood-fill ──────────────────────────────────────
GraphEngine::FloodResult f = g.floodFill("A");

f.reachable       // QStringList — nós que o fluxo alcançou
f.blockedReached  // QStringList — válvulas que o fluxo atingiu (não cruzou)
f.unreachable     // QStringList — nós que o fluxo não alcançou
f.flowEdges       // QList<FlowEdge{from,to,cost}> — arestas com fluxo
f.toString()      // QString formatada para exibição

// ── XML — import / export ───────────────────────────────────────────────────
g.saveToXmlFile("grafo.xml");     // salva em XML
g.loadFromXmlFile("grafo.xml");   // carrega de arquivo XML
g.loadFromXml(xmlBytes);          // carrega de QByteArray
QByteArray xml = g.toXml();       // serializa para QByteArray
```

### Estrutura interna

Internamente todos os nós são indexados por `int` (`NodeId`). `QString` só toca o grafo na fronteira pública — uma única lookup de hash na entrada e reconstrução de labels na saída. Dijkstra e BFS operam exclusivamente sobre inteiros e `double[]` contíguos; strings aparecem apenas num único passe de conversão após o algoritmo terminar.

```
label → NodeId      (QHash, 1× por chamada pública)
m_nodes[id]         (QVector, acesso O(1) por índice)
m_adj[fromId]       (lista de adjacência, itera só vizinhos)
dist[] / prev[]     (std::vector<double/int>, cache-friendly) — Dijkstra
dist[] / blocked[]  (std::vector<int/bool>)                  — floodFill BFS
```

---

## Estrutura do repositório

```
GraphEngine/
├── index.html          # editor visual (abrir no browser)
├── grafo.json          # grafo de exemplo exportado pelo editor
├── GraphEngine.pro     # projeto Qt Creator (subdirs)
├── lib/
│   ├── lib.pro
│   ├── graphengine.h
│   └── graphengine.cpp
└── app/
    ├── app.pro
    └── main.cpp        # demo: carrega grafo.json, testa Dijkstra e floodFill
```

## Formatos de arquivo

### JSON

```json
{
  "version": 1,
  "nodes": [
    { "id": "n1", "x": 100, "y": 200, "label": "Entrada" }
  ],
  "edges": [
    { "id": "e1", "from": "n1", "to": "n2", "cost": 1 }
  ]
}
```

### XML

Mesmo esquema do JSON — editor e biblioteca consomem o mesmo arquivo:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<graphengine version="1">
  <nodes>
    <node id="n1" x="100" y="200" label="Entrada"/>
  </nodes>
  <edges>
    <edge id="e1" from="n1" to="n2" cost="1"/>
  </edges>
</graphengine>
```

> **Nota:** quando exportado pela biblioteca C++, `x` e `y` são `0` (a lib não armazena layout). O editor auto-distribui esses nós em linha ao importar.
