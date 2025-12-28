# Performance Optimization Report — SimpleEngine

**Data**: 2025-12-28  
**Fase**: Prioridade A (otimizações seguras/alto impacto)

---

## Resumo Executivo

Implementadas **5 otimizações de Prioridade A** focadas em reduzir alocações em hot paths e melhorar cache efficiency.

---

## Mudanças Implementadas

### A1: RenderSystem — reserve() para instanceBatches_
**Arquivo**: `engine/src/ecs/RenderSystem.cpp`

- **Problema**: `instanceBatches_[key].push_back(instanceData)` crescia vetores sem reserva prévia
- **Solução**: Implementado tracking de contagens do frame anterior (`lastFrameInstanceCounts_`) para pré-reservar vetores

```cpp
// Antes
for (auto& [key, instances] : instanceBatches_) { instances.clear(); }

// Depois
for (auto& [key, instances] : instanceBatches_) {
    auto lastIt = lastFrameInstanceCounts_.find(key);
    if (lastIt != lastFrameInstanceCounts_.end() && lastIt->second > 0) {
        instances.reserve(lastIt->second);
    }
    instances.clear();
}
```

**Impacto esperado**: Redução de realocações de vetores por frame.

---

### A2: RenderSystem — Cache de Uniform Locations para Bones
**Arquivos**: `engine/include/engine/Shader.h`, `engine/src/Shader.cpp`, `engine/src/ecs/RenderSystem.cpp`

- **Problema**: `snprintf` chamado para cada bone matrix (até 256) em cada frame para formar nome de uniform
- **Solução**: 
  1. Adicionado `getUniformLocation()` com cache em `Shader`
  2. Adicionado `setMat4ByLocation()` para setar uniforms diretamente por location
  3. Cache estático de locations em RenderSystem

```cpp
// Antes (por bone, por frame)
char uniformName[64];
snprintf(uniformName, sizeof(uniformName), "uBoneMatrices[%zu]", i);
shader->setMat4(uniformName, boneMatrices[i]);

// Depois
shader->setMat4ByLocation(boneUniformLocations_[i], boneMatrices[i]);
```

**Impacto esperado**: 
- Eliminados ~256 snprintf por modelo skinned por frame
- Eliminadas ~256 chamadas glGetUniformLocation por modelo skinned por frame

---

### A3: EventChannel — Capacidade Inicial para Vetores
**Arquivo**: `engine/include/engine/events/EventChannel.h`

- **Problema**: Vetores de eventos e listeners cresciam sem capacidade inicial
- **Solução**: Constructor com reservas:
  - `current_events_`: 32
  - `next_events_`: 32
  - `current_listeners_`: 16
  - `pending_listeners_`: 8

**Impacto esperado**: Redução de realocações durante uso normal do EventBus.

---

### A4: PhysicsDebugDraw — Verificação de reserve()
**Arquivo**: `engine/src/physics/PhysicsDebugDraw.cpp`

- **Status**: Já possui `vertices.reserve(lines_.size() * 6)` na linha 72
- **Observação**: Identificado padrão de alocação per-frame de VertexBuffer/VertexArray (linhas 84-89) — candidato para Prioridade B

---

### A5: RenderSystem — Eliminação de Busca Linear por Material/VA
**Arquivos**: `engine/include/engine/ecs/RenderSystem.h`, `engine/src/ecs/RenderSystem.cpp`

- **Problema**: Loop de processamento de batches fazia busca linear O(n) em todas as entidades para encontrar `shared_ptr<Material>` e `shared_ptr<VertexArray>` correspondentes à key
- **Solução**: 
  1. Adicionada struct `BatchResources` para armazenar shared_ptrs e propriedades emissivas
  2. Populado cache `batchResources_` durante a fase de agrupamento (O(1) por entidade)
  3. Substituído loop de busca por lookup direto no hash map (O(1))

```cpp
// Antes (O(n) por batch)
for (auto entity : view) {
    if (meshRender.vertex_array.get() == key.va && ...) {
        material = meshRender.material;
        va = meshRender.vertex_array;
        break;
    }
}

// Depois (O(1))
auto resourceIt = batchResources_.find(key);
const auto& resources = resourceIt->second;
const auto& va = resources.va;
const auto& material = resources.material;
```

**Impacto esperado**: 
- Complexidade de O(batches × entidades) → O(batches + entidades)
- Eliminação de comparação de ponteiros repetida

---

## Prioridade B — Médio Risco

### B1: PerformanceProfiler — string_view Optimization
**Arquivo**: `engine/include/engine/core/PerformanceProfiler.h`

- **Problema**: `BeginSection`/`EndSection` usavam `const std::string&`, causando potencial alocação
- **Problema**: `ScopedTimer` copiava std::string no construtor
- **Solução**: 
  - Mudado para `std::string_view`
  - `ScopedTimer` agora usa `const char*`

**Impacto esperado**: Eliminadas alocações de string para seções de profiling.

### B2: ThreadPool — std::function Allocation (Backlog)
**Status**: Analisado, mantido como backlog

O padrão atual (`std::make_shared<std::packaged_task>` + lambda capturando shared_ptr) requer refactor significativo. Opções futuras:
- `std::move_only_function` (C++23)
- Pool de tasks reutilizáveis
- Custom type-erased callable

### B3: PhysicsDebugDraw — Reutilização de VBO/VAO
**Arquivos**: `engine/include/engine/physics/PhysicsDebugDraw.h`, `engine/src/physics/PhysicsDebugDraw.cpp`

- **Problema**: `std::make_shared<VertexBuffer>` e `std::make_shared<VertexArray>` criados a cada frame no `Flush()`
- **Solução**: 
  1. Pré-alocação de VBO (dynamic) e VAO no construtor com capacidade inicial (512 linhas)
  2. Reutilização via `SetData()` para atualizar buffer existente
  3. Realocação apenas quando capacidade excedida (growth factor 1.5x)
  4. Adicionado wrapper user-friendly (`SetDebugDrawEnabled`, `SetMode`, `DebugDrawMode` enum)

```cpp
// Antes (por frame)
auto vb = std::make_shared<VertexBuffer>(vertices.data(), size);
auto va = std::make_shared<VertexArray>();
va->AddVertexBuffer(vb);

// Depois (reutilização)
if (requiredSize > bufferCapacity_) {
    bufferCapacity_ = static_cast<uint32_t>(requiredSize * 1.5f);
    vertex_buffer_ = std::make_shared<VertexBuffer>(bufferCapacity_);
    // ... layout setup
}
vertex_buffer_->SetData(vertices.data(), requiredSize);
```

**Impacto esperado**: Eliminadas alocações de VBO/VAO por frame quando physics debug está ativo.

---

## Prioridade C — Build

### C1: Release Build Flags
**Arquivo**: `scripts/generate_visual_studio_files_and_build.bat`

- **Status**: ✅ Já configurado
- `BUILD_TYPE=Release` (linha 28)
- MSVC Release inclui `/O2`, `/DNDEBUG` por padrão

---

## Arquivos Modificados

| Arquivo | Tipo de Mudança |
|---------|-----------------|
| `engine/include/engine/Shader.h` | +cache de uniform locations, +setMat4ByLocation |
| `engine/src/Shader.cpp` | +implementações getUniformLocation, setMat4ByLocation |
| `engine/include/engine/ecs/RenderSystem.h` | +boneUniformLocations_, +lastFrameInstanceCounts_, +BatchResources cache |
| `engine/src/ecs/RenderSystem.cpp` | +reserve com frame anterior, +cached bone uniforms, +O(1) batch resource lookup |
| `engine/include/engine/events/EventChannel.h` | +constructor com reserve |
| `engine/include/engine/core/PerformanceProfiler.h` | +string_view, ScopedTimer const char* |
| `engine/include/engine/physics/PhysicsDebugDraw.h` | +DebugDrawMode enum, +wrapper API, +buffer capacity tracking |
| `engine/src/physics/PhysicsDebugDraw.cpp` | +pre-allocated VBO/VAO, +buffer reuse, +wrapper implementations, +drawContactPoint |

---

## Verificação

- ✅ Build Release concluído com sucesso
- ✅ Aplicação inicia corretamente
- ✅ Physics debug draw funcional com Wireframe mode
- ⏳ Baseline de performance (pendente: profiler não configurado)

---

## Próximos Passos (Prioridade B/C)

1. **ThreadPool**: Avaliar `std::move_only_function` (C++23) vs pool de tasks
2. **Baseline completo**: Configurar profiler (Tracy/Superluminal) para métricas before/after
3. **Pooling de InstancedMesh**: Evitar `std::make_shared<InstancedMesh>` no render loop

---

## Notas Técnicas

### Limitações Conhecidas

- Cache de bone uniform locations é global (primeiro shader a inicializar define os locations)
- Se projetos usam múltiplos shaders com layouts diferentes para bones, precisará de cache por shader

### Recomendações

1. Para medição precisa, configurar Tracy ou outro profiler
2. Executar benchmark com cena com muitos objetos instanciados e modelos animados
3. Comparar frame times antes/depois

