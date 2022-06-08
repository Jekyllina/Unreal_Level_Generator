# Unreal_Level_Generator
 Plugin for creating a level starting from a texture

https://www.youtube.com/watch?v=tbh4DX4zomA

- The level is created starting from a texture
- The command to write in the log is "newlevel", arguments are the name for the level and the name of the texture to use
- If a level with that name already exists, it will be changed adding a _ and a number
- The texture must be in the Content director, if not it will appear a Warning that warns that the texture was not found
- You can spawn two differents type of walls, in this example the black pixels in the texture are the green walls and the red pixels the grey walls
- The floor scale depends on the size of the texture
- All the lights are istantiated by code and ordered in the Lighting folder

 Texture example:
 ![Level2](https://user-images.githubusercontent.com/48209077/172736662-22023218-42a1-429a-9433-8467b2dc4176.png)
