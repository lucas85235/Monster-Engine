# Improvements and features

1. improve map editor navigation. When pressing right button with WASD the camera must move as in other editor engines
2. move all the code that is related with character controller, character input and other abstractions to the engine it self. these classes are generic enough to be in the engine.
3. implement skybox rendering with ibl light improving the pbr rendering 
4. implement an easy way to add trigger colliders in the scene to be used as a game interactor
5. make the engine's math part a separated lib. If possible, implement the EA's math library