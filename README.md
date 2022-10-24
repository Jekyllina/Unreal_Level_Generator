# Unreal_Level_Generator
-WORK IN PROGRESS-
 Plugin for creating a level starting from a texture

https://youtu.be/tor4j-OA_mc

- To open Window -> Developer Tools -> Level Creator
- The level is created starting from a texture
- All the lights are istantiated by code and ordered in the Lighting folder
- The floor scale depends on the size of the texture
- You can create your walls blueprints but they must have dimension X = 1 and Y = 1

When you press Create:
- if you don't choose the walls it warns you and asks if you want to use the default walls (green wall for Wall1 (the black pixels), grey wall for Wall2 (the red pixels))
- if you don't choose a texture for the level it warns you
- if you don't insert a name it warns you and you can choose to use a random name (it will be Level_ followed by random numbers)
- if you insert a name with spaces they will be replaced with _ 
- if you create a level with a name that already exists, _ followed by random numbers will be added to the name


You can find the assets in LevelCreatorPlugin Content folder
The plugin already has 2 default walls and 2 textures (Level2 and Level3) 
The levels will be created in the base Content folder
