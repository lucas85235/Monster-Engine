# Improvements and features

1. improve map editor navigation. When pressing right button with WASD the camera must move as in other editor engines
2. move all the code that is related with character controller, character input and other abstractions to the engine it self. these classes are generic enough to be in the engine.
3. implement skybox rendering with ibl light improving the pbr rendering 
4. implement an easy way to add trigger colliders in the scene to be used as a game interactor
5. make the engine's math part a separated lib.
6. implementar uma forma eficiente de renderizar point lights na engine. 
7. implementar técnica de shadow cascades para que funcione de distancias maiores mas de forma mais leve e com debug para vermos possíveis erros.
8. implementar controller input para que funcione com steam deck entre outras plataformas.
9. 