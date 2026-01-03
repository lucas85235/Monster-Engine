# Prompt Técnico: Sistema de Animação Procedural AAA

## Objetivo Central
Desenvolver sistema de animação procedural comparável a títulos AAA (GTA V, The Last of Us, Arc Raiders) com foco em fluidez, naturalidade e performance, preparado para expansão futura com IK e motion matching.

## Fundamentos Teóricos Essenciais

### 1. Teoria de State Machines Hierárquicas
- **Conceito de Sub-State Machines**: Estados complexos contêm suas próprias máquinas de estado internas
- **Herança de Contexto**: Estados filhos herdam propriedades e transições do estado pai
- **Prioridade de Transições**: Sistema de override onde transições mais específicas têm precedência
- **State Layering**: Múltiplas máquinas de estado operando simultaneamente em diferentes níveis de abstração

### 2. Matemática de Interpolação e Blending

#### Quaternion Slerp (Spherical Linear Interpolation)
- **Fundamento**: Interpolação na superfície de uma hiperesfera 4D para rotações suaves
- **Vantagem sobre Euler**: Elimina gimbal lock e produz caminhos mais curtos
- **Otimização**: Usar Nlerp (normalized lerp) quando ângulo < 10° para performance
- **Double Cover Problem**: Sempre escolher o caminho mais curto entre quaternions (dot product check)

#### Blend Space Triangulation (Delaunay)
- **Conceito**: Dividir espaço 2D em triângulos para interpolação baricêntrica
- **Algoritmo**: Dado ponto P dentro de triângulo ABC, calcular pesos (α, β, γ) onde α+β+γ=1
- **Extrapolação**: Quando ponto está fora dos samples, projetar no triângulo mais próximo
- **Caching**: Manter último triângulo usado como hint para próxima busca (coerência temporal)

#### Cross-fade Quality
- **Linear Blending**: Simples mas pode causar "dead zones" (perda de energia)
- **Synchronized Blending**: Alinhar fases de animações cíclicas antes de interpolar
- **Inertial Blending**: Manter velocidade angular/linear durante transição (evita pops)
- **Ease Functions**: Usar curvas sigmoidais (smoothstep, smootherstep) para transições naturais

### 3. Sistemas de Máscaras de Bones

#### Representações Eficientes
- **Bitset Representation**: Cada bone é um bit, operações são AND/OR bitwise
- **Hierarchical Masks**: Máscaras se propagam na hierarquia do esqueleto
- **Soft Masks**: Pesos graduais (0-1) ao invés de binário para borders suaves
- **Precomputed Masks**: Calcular máscaras comuns em tempo de inicialização

#### Blending Multi-Layer
- **Override Mode**: Substituição completa (peso 1.0 = 100% nova pose)
- **Additive Mode**: Delta aplicado sobre base (pose_final = base + delta * weight)
- **Blend Mode**: Interpolação linear (pose_final = lerp(base, overlay, weight))
- **Order of Operations**: Aplicar layers em ordem de prioridade crescente

### 4. Look At e Aim Offset

#### Distributed Rotation Algorithm
- **Princípio**: Distribuir rotação total por múltiplos bones para naturalidade
- **Peso por Bone**: Baseado em mobilidade anatômica real (cervical > torácica > lombar)
- **Constraint Cone**: Cada bone tem cone de rotação máxima permitida
- **Damping**: Aplicar força restauradora proporcional ao desvio do centro

#### Aim Offset como Pose Correction
- **Conceito**: Aim offset é delta aditivo, não pose absoluta
- **Reference Pose**: Sempre partir de pose neutra (olhando para frente)
- **Decomposição**: Separar rotação horizontal e vertical para interpolação independente
- **Clamping Strategies**: Limitar no espaço de entrada (ângulos) vs espaço de saída (rotações)

### 5. Torso Rotation e Body Orientation

#### Hysteresis Pattern
- **Conceito**: Ter thresholds diferentes para ativar/desativar comportamento (evita flickering)
- **Deadzone**: Região onde não há mudança (estabilidade)
- **Attack/Release**: Velocidades diferentes para iniciar vs terminar rotação

#### Lazy Follow Algorithm
- **Spring-Damper System**: Torso segue câmera como massa-mola
- **Stiffness**: Quão rápido corpo reage à mudança de direção
- **Damping**: Prevenir overshooting e oscilações
- **Max Angular Velocity**: Limitar velocidade de rotação para realismo

#### Foot Pivot Detection
- **Heurística**: Se ângulo > threshold e velocidade < epsilon, disparar pivô
- **Transition Timing**: Sincronizar com foot plant na animação para suavidade
- **Blend Out**: Misturar gradualmente nova orientação enquanto pé está plantado

### 6. Strafe e Directional Movement

#### 8-Directional Blending
- **Input Mapping**: Converter input analógico para espaço (forward/right) normalizado
- **Radial Blending**: Usar ângulo como parâmetro principal, magnitude como secundário
- **Diagonal Synthesis**: Interpolar entre cardinais adjacentes ao invés de ter animações diagonais explícitas
- **Speed Warping**: Ajustar playback rate para match entre velocidade de animação e movimento real

#### Camera-Relative Strafe
- **Reference Frame**: Movimento sempre relativo à câmera, não ao personagem
- **Decoupling**: Separar direção de movimento de direção que personagem olha
- **Smooth Transitions**: Quando sair de strafe, blender gradualmente para orientação de movimento
- **Maintained Momentum**: Preservar velocidade vetorial durante mudança de modo

### 7. Animation Layering Architecture

#### Additive Animation Theory
- **Conceito**: Animação aditiva = diferença entre pose animada e pose de referência
- **Extraction**: ref_pose (geralmente T-pose ou primeiro frame) armazenada separadamente
- **Application**: final_pose = base_pose + (additive_pose - ref_pose) * weight
- **Advantages**: Permite combinar múltiplas correções pequenas

#### Layer Composition Order
- **Bottom-Up Evaluation**: Processar layers da base para o topo
- **Early-Out Optimization**: Se layer tem peso 0, skip completamente
- **Partial Skeleton Updates**: Só computar bones afetados por cada layer
- **Deferred Blending**: Acumular operações e executar uma vez ao final

### 8. Performance e Otimização

#### Pose Caching Strategy
- **Conceito**: Poses intermediárias podem ser reutilizadas em múltiplos frames
- **Invalidation**: Detectar quando cache precisa ser limpo (mudança de estado, input)
- **Granularity**: Cachear por layer, por blend space, por state
- **Memory vs Speed Trade-off**: Mais cache = menos CPU mas mais RAM

#### LOD (Level of Detail) Heuristics
- **Distance-based**: Quanto mais longe, menor update rate e menos bones
- **Importance Scoring**: Personagem principal sempre high LOD, NPCs variam
- **Bone Reduction**: Remover bones não essenciais (fingers, toes) em LODs baixos
- **Update Rate Throttling**: High LOD = 60fps, Medium = 30fps, Low = 15fps

#### SIMD Vectorization
- **Batch Processing**: Processar 4 quaternions simultâneos (SSE/AVX)
- **Structure of Arrays**: Separar x,y,z,w em arrays paralelos para vectorização
- **Aligned Memory**: Garantir alinhamento de 16 bytes para SIMD
- **Horizontal Operations**: Minimizar soma/redução cross-lane (são lentas)

#### Multi-threading Patterns
- **Phase Separation**: Input gathering → State update → Pose blending → Application
- **Job System**: Dividir skeleton updates entre workers (character = job unit)
- **Lock-free Design**: Usar atomic operations e double buffering ao invés de mutexes
- **Temporal Coherence**: Usar resultado do frame anterior como hint

### 9. Transition Quality

#### Motion Matching Lite
- **Conceito**: Encontrar melhor ponto de entrada na animação target baseado em pose atual
- **Feature Vector**: Posição de joints chave + velocidades como assinatura de pose
- **Distance Metric**: Weighted sum de diferenças de posição/rotação
- **Search Window**: Limitar busca a intervalos válidos da animação (não no meio de keyframes críticos)

#### Inertial Blending
- **Problema**: Transições abruptas criam "pops" visíveis
- **Solução**: Calcular velocidade de cada joint e mantê-la durante blend
- **Implementation**: velocity = (current_pos - previous_pos) / dt
- **Decay**: Aplicar damping gradual para convergir suavemente à nova animação

#### Synchronized Transitions
- **Phase Matching**: Alinhar walk cycles pela fase (heel strike to heel strike)
- **Time Warping**: Acelerar/desacelerar sutilmente para sincronizar momentos chave
- **Event-Based Triggers**: Esperar por "notification" na animação (foot plant, hand reach, etc)
- **Predictive Blending**: Começar transição antes do trigger para compensar blend time

### 10. Root Motion

#### Extraction Methods
- **In-place vs Locomotion**: Animações podem ter movimento no root bone ou não
- **Velocity Extraction**: Derivar velocidade a partir de deltas de posição frame-a-frame
- **Rotation Extraction**: Extrair yaw do root para steering, manter pitch/roll na animação
- **Gravity Separation**: Separar movimento Y (gravidade) de movimento XZ (locomoção)

#### Application Strategies
- **Full Root Motion**: Movimento vem 100% da animação (cinemático)
- **Partial Root Motion**: Blend entre animação e input physics (mais controle)
- **Warping**: Ajustar root motion para match destino desejado sem alterar animação
- **Prediction**: Extrapolar trajetória futura para sistemas de pathfinding

### 11. Preparação para IK

#### Forward Kinematics Foundation
- **Conceito**: Propagar transformações do root até end effectors
- **Local vs World Space**: Cada bone tem transform local ao parent
- **Accumulation**: World transform = parent_world * local
- **Dirty Flag Propagation**: Marcar hierarquia como "needs update" eficientemente

#### Two-Bone IK (FABRIK simplificado)
- **Use Case**: Braços, pernas (shoulder-elbow-hand, hip-knee-foot)
- **Law of Cosines**: Resolver triângulo formado por upper bone, lower bone, distance to target
- **Pole Vector**: Definir plano onde cotovelo/joelho deve ficar (evita twist)
- **Constraints**: Limitar ângulos para evitar hiperextensão

#### Full-Body IK Considerations
- **Chain Definition**: Identificar cadeias cinemáticas (spine, arms, legs)
- **Priority System**: Alguns effectors têm prioridade sobre outros (hands > elbows)
- **Iterative Refinement**: Múltiplas passadas para convergir solução
- **Soft Constraints**: Permitir desvio se constraint impossível (graceful degradation)

### 12. Data-Driven Configuration

#### Serialization Requirements
- **Human-Readable**: JSON/YAML para designers editarem sem programar
- **Hot-Reload**: Detectar mudanças em arquivo e recarregar em runtime
- **Validation**: Checar consistência (references válidas, ranges corretos)
- **Versioning**: Suportar múltiplas versões do schema para compatibilidade

#### Parametrization Philosophy
- **Exposed Parameters**: O que designers precisam tunar (blend times, thresholds, weights)
- **Hidden Complexity**: Algoritmos complexos ficam em código, não em config
- **Sensible Defaults**: Sistema funciona razoavelmente sem configuração
- **Override Hierarchy**: Defaults globais < per-character < per-instance

### 13. Debug e Visualization

#### Essential Debug Features
- **Skeleton Rendering**: Desenhar bones e joints como linhas/esferas
- **State Visualization**: Mostrar estado atual, transições ativas, blend weights
- **Blend Space Debug**: Plotar posição atual em 2D grid
- **Bone Highlighting**: Colorir bones por layer, mask, ou modificação
- **Trajectory Prediction**: Desenhar caminho futuro baseado em velocidade atual

#### Profiling Strategies
- **Per-Character Timing**: Quanto tempo cada character consome em animation
- **Per-Layer Breakdown**: Identificar layers mais caros
- **Cache Hit Rate**: Quantas vezes poses cacheadas são reutilizadas
- **LOD Distribution**: Quantos characters em cada nível de LOD

## Heurísticas Críticas para Qualidade AAA

### Naturalidade
1. **Nenhuma rotação deve ser instantânea**: Sempre interpolar, mesmo que rápido
2. **Distribuir movimento**: Nunca rotacionar apenas um bone, envolver vizinhos
3. **Respeitar física**: Momentum deve ser preservado, não violado
4. **Variação sutil**: Adicionar noise procedural leve para evitar robótica

### Responsividade
1. **Input buffering**: Aceitar input durante transição, executar quando possível
2. **Canceling**: Permitir cancelar animações longas com animações de alta prioridade
3. **Predictive blending**: Antecipar intenção do jogador baseado em input recente
4. **Latency hiding**: Começar feedback visual imediatamente, mesmo se gameplay atrasado

### Performance
1. **Coerência temporal**: Frame N+1 é similar a frame N, use isso para optimizar
2. **Culling agressivo**: Se não está visível, não calcular
3. **Batch processing**: Agrupar operações similares para melhor cache usage
4. **Lazy evaluation**: Só computar quando resultado é efetivamente usado

### Escalabilidade
1. **Modularidade**: Cada sistema deve funcionar independentemente
2. **Graceful degradation**: Se recurso não disponível, ter fallback razoável
3. **Data-driven**: Lógica em código, parâmetros em dados
4. **Extensibility points**: Permitir injetar comportamento customizado sem modificar core

## Conceitos de Engines Comerciais para Integrar

### Unreal Engine Anim Blueprint
- **Node Graph Paradigm**: Fluxo de dados visual, não código imperativo
- **Cached Poses**: Reutilizar resultado de sub-graphs caros
- **State Machine Aliasing**: Múltiplos estados podem compartilhar mesma animação
- **Anim Notify System**: Eventos sincronizados com timeline de animação

### Unity Animator
- **Any State Transitions**: Estado especial que pode transitar de qualquer lugar
- **Sub-State Machines**: Encapsular complexidade, expor só interface necessária
- **Blend Trees**: Estrutura hierárquica de blend spaces (trees dentro de trees)
- **Avatar Masking**: Retargeting de animações entre rigs diferentes

### RAGE Engine (GTA/RDR)
- **Context-Aware Transitions**: Detectar ambiente e escolher transição apropriada
- **Partial Body IK**: Aplicar IK só em membros necessários
- **Dynamic Reachability**: Calcular se personagem consegue alcançar alvo antes de iniciar animação
- **Procedural Overlays**: Adicionar reações (stumble, flinch) sobre animação base

### Naughty Dog (The Last Of Us)
- **Motion Matching Database**: Buscar melhor animação baseado em contexto completo
- **Trajectory Prediction**: Planejar caminho futuro e escolher animações que facilitam
- **Dynamic Obstacles**: Ajustar movimento proceduralmente para evitar colisões
- **Emotional States**: Modular animações baseado em estado emocional do personagem

## Ordem de Implementação Sugerida

### Fundação (Mais Importante)
1. Sistema de Quaternion robusto com todas operações necessárias
2. State Machine genérica com suporte a hierarquia e transições
3. Skeleton e Pose como estruturas de dados fundamentais
4. Sistema de blending básico (linear interpolation)

### Camada Intermediária
5. Blend Spaces 1D e 2D com triangulação
6. Sistema de máscaras de bones e layers
7. Animation clip playback com sampling correto
8. Root motion extraction e application

### Features Avançadas
9. Look At controller com distribuição de rotação
10. Torso rotation com hysteresis
11. Strafe blending com camera-relative
12. Transition quality (phase matching, inertial blending)

### Polish e Otimização
13. LOD system com distance-based switching
14. Multi-threading do pipeline de animação
15. Debug visualization e profiling tools
16. Preparação arquitetural para IK futuro

## Métricas de Sucesso

- **Fluidez Visual**: Nenhum "pop" ou descontinuidade visível em transições
- **Responsividade**: Input refletido em animação em < 100ms
- **Performance**: Manter 60fps com 10+ personagens animados em high LOD
- **Naturalidade**: Animações indistinguíveis de motion capture real
- **Configurabilidade**: Designers podem ajustar comportamento sem tocar código

Este sistema deve ser a fundação para um character controller de qualidade AAA, comparável aos melhores jogos da indústria.