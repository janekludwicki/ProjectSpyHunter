### Spy Hunter Game Project
This project is a simple 2D car racing game, based on Spy Hunter arcade game, implemented using the SDL library. The objective of the game is to navigate the player's car along a road, avoiding collisions with other vehicles and staying within the road boundaries. The game features smooth scrolling backgrounds, animated explosions, and random vehicle spawns to create a dynamic and engaging gameplay experience.


1. **Overview**

![image](https://github.com/janekludwicki/ProjectSpyHunter/assets/132893147/dc65557c-c059-40bf-b82c-9b21ab6ba85e)


3. **Struct Definitions**
   - **Point**: Represents a coordinate with `x` and `y` values.
   - **colors**: Stores color values for different elements.
   - **Player**: Holds the player's car properties and state.
   - **Vehicles**: Represents other cars (friendlies and enemies) on the road.
   - **MovingBackground**: Manages background images for the road and verges.
   - **explosionAnimation**: Handles the explosion animations.
   - **Game**: Contains all game-related data, including player, vehicles, backgrounds, and SDL objects.

4. **Utility Functions**
   - **DrawString**: Draws text on the screen.
   - **DrawSurface**: Draws an SDL surface (sprite) on the screen at specified coordinates.
   - **DrawPixel**: Draws a single pixel on the screen.
   - **DrawLine**: Draws a line (vertical or horizontal) on the screen.
   - **DrawRectangle**: Draws a filled rectangle with an outline on the screen.
   - **CheckBitmap**: Checks if a bitmap image loaded correctly.
   - **doOverlap**: Checks for collisions between two rectangles.
   - **CreateFreePosition**: Generates a random position for a vehicle ensuring it does not overlap with existing vehicles.
   - **CreateFriendlies**: Initializes friendly vehicles with random positions and sprites.
   - **Load**: Loads all required game assets.
   - **Init**: Initializes SDL and game components.
   - **NewGame**: Resets game state for a new game.
   - **FindFreePosition**: Finds a free position for a new vehicle.
   - **FriendlyCheck**: Checks and updates the state of friendly vehicles.
   - **FriendliesSpawner**: Spawns friendly vehicles.
   - **CollisionsOthers**: Checks for collisions between vehicles.
   - **Collisions**: Checks for collisions between the player's car and other vehicles.
   - **CheckCollisions**: Triggers collision checks.
   - **Explosion**: Manages explosion animations.
   - **Draw**: Renders the game graphics on the screen.

#### Game Flow
   
1. **Collision Detection**:
   - Ensures the player’s car avoids other vehicles and stays within the road boundaries.
   - Triggers explosion animations and penalties on collisions.

2. **Random Vehicle Generation**:
   - Spawns friendly vehicles at random positions with random sprites.
   - Ensures no overlap between newly spawned and existing vehicles.

5. **Graphics and Animations**:
   - Draws the road and verges to create a scrolling background effect.
   - Manages explosion animations for dynamic visual feedback on collisions.
