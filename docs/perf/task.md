# Performance Optimization Task Checklist

**Objetivo**: Análise e otimização de performance/memória da SimpleEngine.

**Plataforma**: Windows (Visual Studio / CMake)  
**Linguagem**: C++23  
**Build Script**: `scripts/generate_visual_studio_files_and_build.bat`

---

## Fase 0 — Inventário de Módulos

### Estrutura do Repositório

| Módulo | Localização | Status | Achados |
|--------|-------------|--------|---------|
| **Core** | `engine/include/engine/core/` | [ ] Pendente | PerformanceProfiler.h, ThreadPool.h, ServiceLocator.h, Time.h |
| **ECS** | `engine/include/engine/ecs/` + `engine/src/ecs/` | [ ] Pendente | RenderSystem (575 linhas), Scene, Entity, Components |
| **Renderer** | `engine/src/Renderer/` | [ ] Pendente | SceneRenderer (1032 linhas), GI passes, Instancing |
| **Resources** | `engine/src/resources/` | [ ] Pendente | ModelManager, TextureManager, MaterialManager, MapLoader |
| **Physics** | `engine/src/physics/` | [ ] Pendente | PhysicsSystem (514 linhas), PhysicsDebugDraw, Bullet integration |
| **Animation** | `engine/src/animation/` | [ ] Pendente | Animator, SkinnedModel, AnimationLoader |
| **Events** | `engine/include/engine/events/` | [ ] Pendente | EventBus, EventChannel (std::function allocations) |
| **Input** | `engine/include/engine/input/` | [ ] Pendente | InputManager |
| **UI** | `engine/src/engine/ui/` | [ ] Pendente | RmlUI integration, Canvas |
| **Application** | `engine/src/Application.cpp` | [ ] Pendente | Main loop (216 linhas) |

---

## Fase 1 — Identificação de Hot Paths

### Hot Paths Identificados

#### 1. RenderSystem::Render() — `engine/src/ecs/RenderSystem.cpp:154-572`
- **Problema A**: `instanceBatches_[key].push_back(instanceData)` (linha 229) sem `reserve()`
- **Problema B**: `snprintf` em loop para uniform names (linhas 287-288, 384-387)
- **Problema C**: Busca linear por material/VA em cada batch (linhas 505-513)
- **Problema D**: `std::make_shared<InstancedMesh>` dentro do frame loop (linhas 531, 541)

#### 2. SceneRenderer::EndScene() — `engine/src/Renderer/SceneRenderer.cpp:173-301`
- **Problema**: Submissions vector manipulation, shader state changes

#### 3. PhysicsSystem::Update() — `engine/src/physics/PhysicsSystem.cpp:182-256`
- **Problema A**: Iteração sobre `bodies_` a cada frame
- **Problema B**: Mutex contention em `physics_mutex_`

#### 4. EventBus/EventChannel — `engine/include/engine/events/`
- **Problema**: `std::function` allocations, vector swap/clear per frame

#### 5. PhysicsDebugDraw — `engine/src/physics/PhysicsDebugDraw.cpp`
- **Problema**: `push_back` repetido em loop (linhas 75-86)

#### 6. TransformComponent — `engine/include/engine/ecs/SimpleComponents.h`
- **Observação**: Já possui dirty-flag caching (bom!)

---

## Fase 2 — Checklist de Otimizações

### Prioridade A — Alto Impacto / Baixo Risco

- [x] **A1**: RenderSystem - Adicionar `reserve()` para `instanceBatches_`
- [x] **A2**: RenderSystem - Cache uniform location IDs ao invés de snprintf por bone
- [x] **A3**: RenderSystem - Evitar busca linear por material (usar cache/mapa direto)
- [x] **A4**: PhysicsDebugDraw - `reserve()` para `vertices` vector
- [x] **A5**: EventChannel - Pre-allocar vetores de eventos com capacidade inicial
- [ ] **A6**: ThreadPool - Considerar usar `std::move_only_function` (C++23) ou function_ref

### Prioridade B — Médio Risco / Alto Ganho

- [ ] **B1**: Introduzir Frame Allocator (arena per-frame) para temporários
- [ ] **B2**: Substituir `std::shared_ptr` por raw pointers onde ownership é clara
- [x] **B3**: Cache de uniform locations para todos shaders (parcial: bones apenas)
- [x] **B4**: Pooling de InstancedMesh ao invés de realocação (já implementado via instancedMeshCache_)
- [x] **B5**: PhysicsDebugDraw - Reutilizar VBO/VAO ao invés de alocar por frame
- [x] **B6**: PerformanceProfiler - string_view para section names

### Prioridade C — Build/Otimizações Avançadas

- [x] **C1**: Verificar Release build flags (LTO, /O2) — já configurado
- [x] **C2**: Remover logs em hot paths (gated por constexpr) — já usa pattern de log periódico
- [ ] **C3**: Verificar RTTI/exceptions impact

---

## Fase 3 — Métricas e Validação

### Baseline a Coletar

- [ ] Frame time médio, p95, p99
- [ ] Draw calls por frame
- [ ] Alocações por frame (instrumentar allocator se necessário)
- [ ] CPU hotspots (profiler)

### Comandos de Build/Run

```batch
# Build
scripts\generate_visual_studio_files_and_build.bat

# Run (third_person_game)
build\Debug\third_person_game.exe
# ou
build\Release\third_person_game.exe
```

---

## Fase 4 — Relatórios Finais

- [ ] `docs/perf/baseline.md` — Métricas antes das otimizações
- [ ] `docs/perf/report.md` — Comparativo before/after
- [ ] `docs/perf/allocations.md` — Estratégia de memória

---

## Notas Técnicas

### Padrões de Alocação Encontrados

1. **shared_ptr extensivo**: MeshRenderComponent, Materials, VertexArrays
2. **std::vector sem reserve**: `instanceBatches_`, `lines_` (debug draw)
3. **String formatting em loops**: snprintf para uniforms (bones)
4. **std::function**: EventChannel, ThreadPool

### Estruturas de Dados

- `TransformComponent`: 116 bytes (Matrix4 cached + Quaternion + dirty flags) — **OK**
- `MeshRenderComponent`: ~80 bytes (2x shared_ptr + Vector4 + bools + emissive)
- `DirectionalLightComponent`: ~20 bytes — **OK**

### Concorrência

- PhysicsSystem: Bullet MT internal threading
- ThreadPool: shared task queue com mutex
- EventBus: dispatch no main thread

