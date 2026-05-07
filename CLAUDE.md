# CLAUDE.md — GraphEngine

## O que é este projeto

Dois componentes que se comunicam via JSON:

1. **`index.html`** — editor visual de grafos (HTML5 + Canvas, sem dependências)
2. **`lib/` + `app/`** — biblioteca C++ estática (Qt 6) com Dijkstra e BFS flood-fill

## Estrutura

```
GraphEngine/
├── index.html          # editor — tudo num arquivo só
├── grafo.json          # grafo de exemplo
├── GraphEngine.pro     # subdirs: lib e app
├── lib/
│   ├── lib.pro
│   ├── graphengine.h
│   └── graphengine.cpp
└── app/
    ├── app.pro
    └── main.cpp
```

## Decisões de design importantes

### Biblioteca C++

- **NodeId = int** internamente. `QString` só toca o grafo nas funções públicas (uma hash lookup na entrada, reconstrução de labels na saída). O Dijkstra roda 100% em `int` e `double[]`.
- **Lista de adjacência** (`m_adj[fromId]`): o Dijkstra itera só os vizinhos do nó atual, não todas as arestas.
- **`std::vector<double>` para `dist[]`**: array contíguo, favorece cache no loop crítico.
- `QVector` é alias de `QList` no Qt 6 — usar `emplace_back()` em vez de `append({})` para evitar ambiguidade de overload.
- **`floodFill` — strings apenas na fronteira**: o loop BFS opera 100% em `NodeId` (int) e `bool[]`. A conversão NodeId → label acontece num único passe O(N+E) após o BFS terminar. Nunca acessar `m_nodes[id].label` dentro do loop quente.

### Editor HTML

- **Estado global em `G`** (`nodes`, `edges`, `nextNode`, `nextEdge`, `labelIdx`).
- **Auto-save** chama `localStorage.setItem` a cada mutação — não tem debounce intencional (grafo pequeno).
- **Popup de aresta**: ao abrir (`openEdgePopup`), usar `e.stopPropagation()` no `mousedown` do canvas para evitar que o listener `document.mousedown` feche o popup imediatamente.
- **Labels de nó** não têm limite de tamanho.

## Como buildar a lib

Abrir `GraphEngine.pro` no Qt Creator (Qt 6, MSVC 2022 ou MinGW). O projeto `app` depende de `lib` — o Qt Creator resolve a ordem automaticamente.

O `app.pro` copia `grafo.json` ao lado do executável via `QMAKE_POST_LINK`.

## API pública da biblioteca

```cpp
GraphEngine g;
g.loadFromFile("grafo.json");   // carrega JSON do editor
g.addNode("X");                 // adiciona nó
g.addEdge("A", "B", 1.0);      // aresta direcionada A→B
g.blockNode("B");               // bloqueia em runtime
g.clearBlocked();               // desbloqueia todos
auto r = g.findPath("A", "C"); // Dijkstra → PathResult{found, path, totalCost, steps}

auto f = g.floodFill("A");    // BFS flood → FloodResult
f.reachable       // QStringList — nós alcançados pelo fluxo
f.blockedReached  // QStringList — válvulas que o fluxo atingiu (mas não cruzou)
f.unreachable     // QStringList — nós que o fluxo não alcançou
f.flowEdges       // QList<FlowEdge{from,to,cost}> — arestas com fluxo
```

## O que NÃO fazer

- Não usar `QString` como chave em operações internas do grafo — o objetivo é manter strings fora dos hot paths.
- Não adicionar limite de tamanho nos labels de nós (foi removido intencionalmente).
- Não usar `append({})` em `QVector<QVector<T>>` no Qt 6 — usar `emplace_back()`.
- Não acessar `m_nodes[id].label` dentro de loops BFS/Dijkstra — strings só na fronteira pública (entrada via `resolve()`, saída no passe final).
