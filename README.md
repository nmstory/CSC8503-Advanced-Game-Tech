# C++ Physics & AI Coursework

This coursework involved producing a game similar in the style of [_Screwball Scramble_](https://www.youtube.com/watch?v=JM1Kf70XRa8), where the player can only control the environment, not the ball, in order to progress through the game. 

This game is split into part A and B:

## Part A - Chase the Time
The main objective for the game in part A of this coursework is for the player to reach their ball to the otherside of the course in the fastest amount of time.

This part utilises:
- Player movement
	- Movement
	- Rotation
	- Forces
	- Friction
- Collision Detection
	- Ray against AABB, OBB, Sphere and Capsule
	- AABB against AABB, Sphere and Capsule
	- Sphere against Sphere, AABB, OBB and Capsule
	- OBB against Sphere
- Collision Resolution
	- Projection method
	- Impulse method
	- Penalty method
- Constraints
	- Using constraints to power various challenges (i.e. slippery balancing board and wrecking ball)
- Gameplay effects
	- Menu's
	- Obstacles

## Part B - The Evil Maze
Here, the player aims to collect more coins than the enemy (AI) player. However, as well as collect coins, the enemy can also choose to chase the player in order to win.

This part utilises:
- Advanced AI
	- Behaviour tree to represent logic
- Pathfinding
	- Grid-based pathfinding using A*
- Spatial Acceleration Structures
	- Using Broad/Narrow phase for more efficient collision detection
- Technology from part A

## YouTube video
Most of the functionality is demonstrated in the following [_demo video_](https://www.youtube.com/watch?v=TF0Lw0-7Nfw)
