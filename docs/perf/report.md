# Performance Optimization Report — SimpleEngine

**Data**: 2025-12-28  
**Fase**: Prioridade A (otimizações seguras/alto impacto)

---

## Resumo Executivo

Implementadas **4 otimizações de Prioridade A** focadas em reduzir alocações em hot paths e melhorar cache efficiency.

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

## Arquivos Modificados

| Arquivo | Tipo de Mudança |
|---------|-----------------|
| `engine/include/engine/Shader.h` | +cache de uniform locations, +setMat4ByLocation |
| `engine/src/Shader.cpp` | +implementações getUniformLocation, setMat4ByLocation |
| `engine/include/engine/ecs/RenderSystem.h` | +boneUniformLocations_, +lastFrameInstanceCounts_ |
| `engine/src/ecs/RenderSystem.cpp` | +reserve com frame anterior, +cached bone uniforms |
| `engine/include/engine/events/EventChannel.h` | +constructor com reserve |

---

## Verificação

- ✅ Build Release concluído com sucesso
- ✅ Aplicação inicia corretamente
- ⏳ Baseline de performance (pendente: profiler não configurado)

---

## Próximos Passos (Prioridade B)

1. **PerformanceProfiler**: Substituir `std::string` por `string_view` para section keys
2. **ThreadPool**: Avaliar `std::move_only_function` (C++23) vs pool de tasks
3. **PhysicsDebugDraw**: Reutilizar VB/VA ao invés de alocar por frame
4. **Baseline completo**: Configurar profiler (Tracy/Superluminal) para métricas before/after

---

## Notas Técnicas

### Limitações Conhecidas

- Cache de bone uniform locations é global (primeiro shader a inicializar define os locations)
- Se projetos usam múltiplos shaders com layouts diferentes para bones, precisará de cache por shader

### Recomendações

1. Para medição precisa, configurar Tracy ou outro profiler
2. Executar benchmark com cena com muitos objetos instanciados e modelos animados
3. Comparar frame times antes/depois

