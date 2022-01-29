#include "CourseworkGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include "../CSC8503Common/Constraint.h"
#include "../CSC8503Common/NavigationGrid.h"
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace NCL;
using namespace CSC8503;

CourseworkGame::CourseworkGame(Window* w) {
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);
	window		= w;

	forceMagnitude	= 10.0f;

	Debug::SetRenderer(renderer);
	physics->UseGravity(true);
	Window::GetWindow()->ShowOSPointer(true);
	Window::GetWindow()->LockMouseToWindow(true);

	InitialiseAssets();
}

void CourseworkGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh"		 , &cubeMesh);
	loadFunc("sphere.msh"	 , &sphereMesh);
	loadFunc("capsule.msh"	 , &capsuleMesh);
	loadFunc("coin.msh"		 , &coinMesh);

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	MainMenu();
}

CourseworkGame::~CourseworkGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete capsuleMesh;
	delete coinMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;

	delete playerSphere;
	delete enemySphere;
	
	delete rootSequence;
	delete aiTarget;

	delete selectionObject;
	delete forwardObject;
}

std::string CourseworkGame::GetTime() {
	// with help from: https://www.py4u.net/discuss/122330
	using namespace std::chrono;

	std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - gameTime;
	std::chrono::milliseconds millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_seconds);

	auto secs = duration_cast<seconds>(millisecs);
	millisecs -= duration_cast<milliseconds>(secs);
	auto mins = duration_cast<minutes>(secs);
	secs -= duration_cast<seconds>(mins);

	std::stringstream ss;
	ss << std::setw(2) << std::setfill('0') << mins.count() << ":" << std::setw(2) << std::setfill('0') << secs.count();
	return ss.str();
}

void CourseworkGame::DisplayPathfinding() {
	for (int i = 1; i < pathfindingNodes.size(); ++i) {
		Vector3 a = pathfindingNodes[i - 1];
		a.y += 15;
		Vector3 b = pathfindingNodes[i];
		b.y += 15;

		Debug::DrawLine(a, b, Vector4(1.0f, 0.0f, 0.0f, 1.0f));
	}
}

void CourseworkGame::UpdateGame(float dt) {
	world->GetMainCamera()->UpdateCamera(dt);
	UpdateKeys();

	SelectObject();
	physics->Update(dt);

	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0.0f, 1.0f, 0.0f));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler();

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);
	}

	world->UpdateWorld(dt);
	renderer->Update(dt);

	// Level specific functionality
	if (currentLevel == 1) {
		L1Gameplay();
	}
	if (currentLevel == 2) {
		L2Gameplay(dt);
	}

	Debug::FlushRenderables(dt);
	renderer->Render();

	// Display main menu
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE)) {
		MainMenu();
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::R)) { // Respawn
		SetGameState(currentLevel);
	}
}

void CourseworkGame::SetGameState(int value) {
	if (value == 1) { // Play level 1
		LoadLevel1();
	}
	else if (value == 2) { // Play level 2
		LoadLevel2();
	}
	else if (value == 3) { // Quit
		Window::DestroyGameWindow();
		exit(0);
	}
}

void CourseworkGame::UpdateKeys() {
	// Shuffling the order of constraints to reduce bias in calculations	
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
}

void CourseworkGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0.0f, 1.0f, 0.0f), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Vector3 charForward  = lockedObject->GetTransform().GetOrientation() * Vector3(0.0f, 0.0f, 1.0f);
	Vector3 charForward2 = lockedObject->GetTransform().GetOrientation() * Vector3(0.0f, 0.0f, 1.0f);

	float force = 100.0f;

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		lockedObject->GetPhysicsObject()->AddForce(-rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		Vector3 worldPos = selectionObject->GetTransform().GetPosition();
		lockedObject->GetPhysicsObject()->AddForce(rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		lockedObject->GetPhysicsObject()->AddForce(fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		lockedObject->GetPhysicsObject()->AddForce(-fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NEXT)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0,-10,0));
	}
}

void CourseworkGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(1000.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60.0f, 40.0f, 60.0f));
	lockedObject = nullptr;
}

void CourseworkGame::LoadLevel1() {
	world->ClearAndErase();
	physics->Clear();
	pistonPlatforms.clear();
	currentLevel = 1;
	gameTime = std::chrono::system_clock::now();
	world->GetMainCamera()->SetPosition(Vector3(-117.6f, 16.6f, -31.8f));
	world->GetMainCamera()->SetPitch(-10.3f);
	world->GetMainCamera()->SetYaw(238.0f);

	// Gameplay ball
	playerSphere = AddSphereToWorld(Vector3(0.0f, 0.0f, 00.0f), 4, 2);
	//playerSphere = AddSphereToWorld(Vector3(20.0f, -100.0f, 380.0f), 4.0f, 2.0f); // skip to further forward in the world
	//playerSphere = AddSphereToWorld(Vector3(-180, -152.0f, 380), 4, 2); // skip to further forward in the world

	// Starting platforms
	AddOBBFloorToWorld(Vector3(0.0f, -30.0f, 40.0f), Vector3(20.0f, 0.0f, 0.0f), Vector3(25.0f, 2.0f, 75.0f));
	AddBalancingPlatformToWorld(Vector3(0.0f, -29.0f, 171.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f, 2.0f, 60.0f), Vector4(0.5294f, 0.8078f, 0.9803f, 0.8f), false, 0.5f, 0.5f);

	AddOBBFloorToWorld(Vector3(20.0f, -90.0f, 285.0f), Vector3(10.0f, 15.0f, -2.0f), Vector3(30.0f, 2.0f, 75.0f));
	AddOBBFloorToWorld(Vector3(39.0f, -103.0f, 380.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(30.0f, 2.0f, 30.0f));
	AddPistonPlatformToWorld(Vector3(71.0f, -72.0f, 380.0f), Vector3(0.0f, 0.0f, 90.0f), Vector3(30.0f, 2.0f, 30.0f), Vector3(-1.0f, 0.0f, 0.0f));

	AddOBBFloorToWorld(Vector3(-70.0f, -134.0f, 380.0f), Vector3(0.0f, 0.0f, 20.0f), Vector3(85.0f, 2.0f, 30.0f));
	AddOBBFloorToWorld(Vector3(-180.0f, -163.0f, 380.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(30.0f, 2.0f, 30.0f));
	AddPistonPlatformToWorld(Vector3(-180.0f, -131.0f, 351.0f), Vector3(0.0f, 90.0f, 90.0f), Vector3(30.0f, 2.0f, 30.0f), Vector3(0.0f, 0.0f, 1.0f));
	AddOBBFloorToWorld(Vector3(-180.0f, -215.0f, 496.0f), Vector3(30.0f, 0.0f, 0.0f), Vector3(30.0f, 2.0f, 100.0f));

	AddCapsuleToWorld(Vector3(-160.0f, -240.0f, 590.0f), 20.0f, 8.0f, 0.0f, true);
	AddCapsuleToWorld(Vector3(-200.0f, -240.0f, 590.0f), 20.0f, 8.0f, 0.0f, true);
	AddAABBFloorToWorld(Vector3(-180.0f, -265.0f, 610.0f), Vector3(30.0f, 2.0f, 30.0f), Vector4(1.0f,1.0f,0.0f,1.0f), true);

	InitialiseWreckingBall();
}

void CourseworkGame::UpdateWreckingBall() {
	if (wreckingBallEnd->GetTransform().GetPosition().y < -117.0f) {
		((rand() % 2) == 0) ? wreckingBallEnd->GetPhysicsObject()->ApplyLinearImpulse(Vector3(0.0f, -10.0f, -60.0f)) : wreckingBallEnd->GetPhysicsObject()->ApplyLinearImpulse(Vector3(0.0f, -10.0f, 60.0f));
	}
}

void CourseworkGame::L1Gameplay() {
	renderer->DrawString("Time elapsed: " + GetTime(), Vector2(20.0f, 90.0f));

	UpdateWreckingBall();

	Vector3 currentPos = playerSphere->GetTransform().GetPosition();

	if (currentPos.z > 590.0f) { // Won the game
		currentLevel = 0;
		MainMenu("You win! Your time was: " + GetTime(), Vector4(0.0f,1.0f,0.0f,1.0f));
	}

	if (currentPos.y < -280.0f) { // Fallen off the edge, respawn
		LoadLevel1();
	}
}

void CourseworkGame::Pathfinding(Vector3 startPos, Vector3 endPos) {
	pathfindingNodes.clear();
	NavigationGrid grid("Part2NavigationGrid.txt");
	NavigationPath outPath;

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		pathfindingNodes.push_back(pos);
	}
}

void CourseworkGame::LoadLevel2() {
	world->ClearAndErase();
	physics->Clear();
	pistonPlatforms.clear();
	coins.clear();
	currentLevel = 2;
	GenerateAIBehaviour();
	world->GetMainCamera()->SetPosition(Vector3(23.4f, 571.6f, -74.8f));
	world->GetMainCamera()->SetPitch(-82.72f);
	world->GetMainCamera()->SetYaw(180.7f);

	// Gameplay ball
	playerSphere = AddSphereToWorld(Vector3(-160.0f, 10.0f, 140.0f), 5.0f, 3.0f); // player starting in top right
	playerSphere->GetRenderObject()->SetColour(Vector4(0.0f, 1.0f, 0.0f, 1.0f));
	playerSphere->SetName("Player");
	enemySphere = AddSphereToWorld(Vector3(140.0f, 10.0f, -160.0f), 5.0f, 3.0f); // starting in bottom left
	enemySphere->GetRenderObject()->SetColour(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
	playerSphere->SetName("Enemy");
		
	Vector4 wallColour(1.0f, 1.0f, 0.0f, 1.0f);
	Vector4 floorColour(0.0f, 1.0f, 1.0f, 1.0f);

	// Floor
	AddAABBFloorToWorld(Vector3(0.0f, 0.0f, 0.0f), Vector3(200.0f, 2.0f, 200.0f), floorColour);

	// Outer walls
	AddOBBFloorToWorld(Vector3(190.0f, 10.0f, 0.0f), Vector3(90.0f, 180.0f, 90.0f), Vector3(200.0f, 10.0f, 20.0f), wallColour);
	AddOBBFloorToWorld(Vector3(-190.0f, 10.0f, 0.0f), Vector3(90.0f, 180.0f, 90.0f), Vector3(200.0f, 10.0f, 20.0f), wallColour);
	AddOBBFloorToWorld(Vector3(0.0f, 10.0f, 190.0f), Vector3(90.0f, 180.0f, 0.0f), Vector3(200.0f, 10.0f, 20.0f), wallColour);
	AddOBBFloorToWorld(Vector3(0.0f, 10.0f, -190.0f), Vector3(90.0f, 180.0f, 0.0f), Vector3(200.0f, 10.0f, 20.0f), wallColour);

	// Next inner
	AddOBBFloorToWorld(Vector3(0.0f, 10.0f, 140.0f), Vector3(90.0f, 180.0f, 0.0f), Vector3(150.0f, 10.0f, 20.0f), wallColour); // top-side
	AddOBBFloorToWorld(Vector3(140.0f, 10.0f, 0.0f), Vector3(90.0f, 180.0f, 90.0f), Vector3(150.0f, 10.0f, 20.0f), wallColour); // left-side
	AddOBBFloorToWorld(Vector3(75.0f, 10.0f, -140.0f), Vector3(90.0f, 180.0f, 0.0f), Vector3(75.0f, 10.0f, 20.0f), wallColour); // bottom-side left
	AddOBBFloorToWorld(Vector3(-100.0f, 10.0f, -140.0f), Vector3(90.0f, 180.0f, 0.0f), Vector3(50.0f, 10.0f, 20.0f), wallColour); // bottom-side right
	AddOBBFloorToWorld(Vector3(-140.0f, 10.0f, -50.0f), Vector3(90.0f, 180.0f, 90.0f), Vector3(100.0f, 10.0f, 20.0f), wallColour); // right-side lower
	AddOBBFloorToWorld(Vector3(-140.0f, 10.0f, 150.0f), Vector3(90.0f, 180.0f, 90.0f), Vector3(50.0f, 10.0f, 20.0f), wallColour); // right-side upper

	// Next inner
	AddOBBFloorToWorld(Vector3(0.0f, 10.0f, -90.0f), Vector3(90.0f, 180.0f, 0.0f), Vector3(100.0f, 10.0f, 20.0f), wallColour); // bottom-side
	AddOBBFloorToWorld(Vector3(90.0f, 10.0f, -75), Vector3(90.0f, 180.0f, 90.0f), Vector3(75, 10.0f, 20.0f), wallColour); // left-side lower
	AddOBBFloorToWorld(Vector3(90.0f, 10.0f, 75), Vector3(90.0f, 180.0f, 90.0f), Vector3(25, 10.0f, 20.0f), wallColour); // left-side upper
	AddOBBFloorToWorld(Vector3(75.0f, 10.0f, 90.0f), Vector3(90.0f, 180.0f, 0.0f), Vector3(25, 10.0f, 20.0f), wallColour); // top-side left
	AddOBBFloorToWorld(Vector3(-50.0f, 10.0f, 90.0f), Vector3(90.0f, 180.0f, 0.0f), Vector3(50.0f, 10.0f, 20.0f), wallColour); // top-side right
	AddOBBFloorToWorld(Vector3(-90.0f, 10.0f, 50.0f), Vector3(90.0f, 180.0f, 90.0f), Vector3(50.0f, 10.0f, 20.0f), wallColour); // right-side upper
	AddOBBFloorToWorld(Vector3(-90.0f, 10.0f, -75.0f), Vector3(90.0f, 180.0f, 90.0f), Vector3(25.0f, 10.0f, 20.0f), wallColour); // right-side lower

	//Inner-most
	AddOBBFloorToWorld(Vector3(0.0f, 10.0f, 40.0f), Vector3(90.0f, 180.0f, 0.0f), Vector3(50.0f, 10.0f, 20.0f), wallColour); // top-side
	AddOBBFloorToWorld(Vector3(40.0f, 10.0f, 0.0f), Vector3(90.0f, 180.0f, 90.0f), Vector3(50.0f, 10.0f, 20.0f), wallColour); // left-side
	AddOBBFloorToWorld(Vector3(-40.0f, 10.0f, 0.0f), Vector3(90.0f, 180.0f, 90.0f), Vector3(50.0f, 10.0f, 20.0f), wallColour); // right-side
	AddOBBFloorToWorld(Vector3(-25.0f, 10.0f, -40.0f), Vector3(90.0f, 180.0f, 0.0f), Vector3(25.0f, 10.0f, 20.0f), wallColour); // bottom-side
	
	// Coins
	AddCoinToWorld(Vector3(160.0f, 10.0f, 160.0f));
	AddCoinToWorld(Vector3(160.0f, 10.0f, -30.0f));
	AddCoinToWorld(Vector3(-10.0f, 10.0f, 160.0f));
	AddCoinToWorld(Vector3(-10.0f, 10.0f, -160.0f));
	AddCoinToWorld(Vector3(-160.0f, 10.0f, -30.0f));
	AddCoinToWorld(Vector3(110.0f, 10.0f, 110.0f));
	AddCoinToWorld(Vector3(110.0f, 10.0f, 20.0f));
	AddCoinToWorld(Vector3(110.0f, 10.0f, -70.0f));
	AddCoinToWorld(Vector3(-110.0f, 10.0f, 110.0f));
	AddCoinToWorld(Vector3(-110.0f, 10.0f, -10.0f));
	AddCoinToWorld(Vector3(60.0f, 10.0f, -10.0f));
	AddCoinToWorld(Vector3(60.0f, 10.0f, 60.0f));
	AddCoinToWorld(Vector3(-60.0f, 10.0f, -60.0f));
	AddCoinToWorld(Vector3(-60.0f, 10.0f, 60.0f));
	AddCoinToWorld(Vector3(10.0f, 10.0f, 10.0f));
	AddCoinToWorld(Vector3(-10.0f, 10.0f, -10.0f));
}

void CourseworkGame::L2Gameplay(float dt) {
	// Update text on screen
	renderer->DrawString("Player score: " + std::to_string(playerSphere->GetScore()), Vector2(5.0f, 10.0f), Vector4(0.0f, 1.0f, 0.0f, 1.0f), 25.0f);
	renderer->DrawString("Enemy score: " + std::to_string(enemySphere->GetScore()), Vector2(55.0f, 10.0f), Vector4(1.0f, 0.0f, 0.0f, 1.0f), 25.0f);

	// Show the grid to debug
	/*
	NavigationGrid grid("Part2NavigationGrid.txt");
	grid.DrawMap(renderer);
	DisplayPathfinding();
	*/

	// Remove coins to be deleted
	auto it = coins.begin();
	while (it != coins.end())
	{
		if ((*it)->isToDelete())
		{
			world->RemoveGameObject(*it, false);
			it = coins.erase(it);
		}
		else {
			++it;
		}
	}

	// Update keys
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		playerSphere->GetPhysicsObject()->ApplyLinearImpulse(Vector3(0.0f, -1.0f, 1.0f));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		playerSphere->GetPhysicsObject()->ApplyLinearImpulse(Vector3(1.0f, -1.0f, 0.0f));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		playerSphere->GetPhysicsObject()->ApplyLinearImpulse(Vector3(0.0f, -1.0f, -1.0f));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		playerSphere->GetPhysicsObject()->ApplyLinearImpulse(Vector3(-1.0f, -1.0f, 0.0f));
	}

	UpdateAIBehaviour(dt);

	Vector3 playerPos = playerSphere->GetTransform().GetPosition();
	if (coins.size() == 0) { // Won the game
		currentLevel = 0;
		if (playerSphere->GetScore() > enemySphere->GetScore()) {
			MainMenu("You win! Your score was: " + playerSphere->GetScore(), Vector4(0.0f, 1.0f, 0.0f, 1.0f));
		}
		else {
			MainMenu("Try again! Your score was: " + playerSphere->GetScore(), Vector4(0.0f, 1.0f, 0.0f, 1.0f));

		}
	}
	if (playerPos.y < -280.0f) { // Fallen off the edge, respawn
		LoadLevel2();
	}
}

GameObject* CourseworkGame::AddCoinToWorld(const Vector3& position) {
	GameObject* coin = new GameObject();
	coin->SetName("coin");

	SphereVolume* volume = new SphereVolume(5.0f);
	coin->SetBoundingVolume((CollisionVolume*)volume);
	coin->GetTransform()
		.SetScale(Vector3(1.0f, 1.0f, 1.0f))
		.SetPosition(position);

	coin->SetRenderObject(new RenderObject(&coin->GetTransform(), coinMesh, nullptr, basicShader));
	coin->SetPhysicsObject(new PhysicsObject(&coin->GetTransform(), coin->GetBoundingVolume()));

	coin->GetPhysicsObject()->SetInverseMass(0.0f);
	coin->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(coin);
	coins.push_back(coin);
	return coin;
}

void CourseworkGame::InitialiseWreckingBall() {
	Vector3 cubeSize = Vector3(5.0f, 5.0f, 5.0f);
	
	float invCubeMass = 5.0f; // how heavy the middle pieces are
	int numLinks = 6;
	float maxDistance = 15.0f; // constraint distance
	float cubeDistance = 10.0f; // distance between links
	
	Vector3 startPos = Vector3(-80.0f, -18.0f, 380.0f);
	
	GameObject* start = AddCubeToWorld(startPos + Vector3(0.0f, 0.0f, 0.0f), cubeSize, 0.0f);
	wreckingBallEnd = AddCubeToWorld(startPos + Vector3(0.0f, (numLinks + 2.0f) * -cubeDistance, 0.0f), cubeSize, invCubeMass);
	
	GameObject* previous = start;
	
	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos + Vector3(0.0f, (i + 1) * -cubeDistance, 0.0f), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}
	PositionConstraint* constraint = new PositionConstraint(previous, wreckingBallEnd, maxDistance);
	world->AddConstraint(constraint);
}

GameObject* CourseworkGame::AddAABBFloorToWorld(const Vector3& position, const Vector3& floorSize, const Vector4& colour, bool isSpring) {
	GameObject* floor = new GameObject();
	floor->SetName("AABB Floor");

	AABBVolume* volume	= new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->GetRenderObject()->SetColour(colour);
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0.0f);
	floor->GetPhysicsObject()->InitCubeInertia();
	if (isSpring) floor->GetPhysicsObject()->SetCollisionType(CollisionType::Spring);

	world->AddGameObject(floor);

	return floor;
}

GameObject* CourseworkGame::AddOBBFloorToWorld(const Vector3& position, const Vector3& rotation, const Vector3& floorSize, const Vector4& colour, bool isSpring, float elasticity, float friction) {
	GameObject* floor = new GameObject();
	floor->SetName("OBB Floor");

	OBBVolume* volume	= new OBBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position)
		.SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), rotation.x) *
						Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 1.0f, 0.0f), rotation.y) *
						Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, 1.0f), rotation.z));

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->GetRenderObject()->SetColour(colour);
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));
	floor->GetPhysicsObject()->SetFriction(friction);
	floor->GetPhysicsObject()->SetElasticity(elasticity);

	floor->GetPhysicsObject()->SetInverseMass(0.0f);
	floor->GetPhysicsObject()->InitCubeInertia();
	if (isSpring) floor->GetPhysicsObject()->SetCollisionType(CollisionType::Spring);

	world->AddGameObject(floor);

	return floor;
}

GameObject* CourseworkGame::AddBalancingPlatformToWorld(const Vector3& position, const Vector3& rotation, const Vector3& floorSize, const Vector4& colour, bool isSpring, float elasticity, float friction) {
	GameObject* platform = new GameObject();
	platform->SetName("Balancing Platform");

	OBBVolume* volume = new OBBVolume(floorSize);
	platform->SetBoundingVolume((CollisionVolume*)volume);
	platform->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position)
		.SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), rotation.x) *
						Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 1.0f, 0.0f), rotation.y) *
						Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, 1.0f), rotation.z));

	platform->SetRenderObject(new RenderObject(&platform->GetTransform(), cubeMesh, basicTex, basicShader));
	platform->GetRenderObject()->SetColour(colour);
	platform->SetPhysicsObject(new PhysicsObject(&platform->GetTransform(), platform->GetBoundingVolume()));

	platform->GetPhysicsObject()->SetInverseMass(0.25f);
	platform->GetPhysicsObject()->SetElasticity(elasticity);
	platform->GetPhysicsObject()->SetFriction(friction);
	platform->GetPhysicsObject()->InitCubeInertia();
	if (isSpring) platform->GetPhysicsObject()->SetCollisionType(CollisionType::Spring);

	BalancingPlaneConstraint* pc = new BalancingPlaneConstraint(platform, Vector3(0.0f, 1.0f, 0.0f));
	world->AddConstraint(pc);

	world->AddGameObject(platform);
	return platform;
}

GameObject* CourseworkGame::AddPistonPlatformToWorld(const Vector3& position, const Vector3& rotation, const Vector3& floorSize, const Vector3& pistonMovementConstraint, bool isSpring) {
	GameObject* platform = new GameObject();
	platform->SetName("Piston Platform");

	OBBVolume* volume = new OBBVolume(floorSize);
	platform->SetBoundingVolume((CollisionVolume*)volume);
	platform->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position)
		.SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), rotation.x) *
						Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 1.0f, 0.0f), rotation.y) *
						Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, 1.0f), rotation.z));

	platform->SetRenderObject(new RenderObject(&platform->GetTransform(), cubeMesh, basicTex, basicShader));
	platform->SetPhysicsObject(new PhysicsObject(&platform->GetTransform(), platform->GetBoundingVolume()));

	platform->GetPhysicsObject()->SetInverseMass(0.25f);
	platform->GetPhysicsObject()->InitCubeInertia();
	if (isSpring) platform->GetPhysicsObject()->SetCollisionType(CollisionType::Spring);

	PistonConstraint* pc = new PistonConstraint(platform, pistonMovementConstraint);
	world->AddConstraint(pc);
	pistonPlatforms.push_back(platform);

	world->AddGameObject(platform);
	return platform;
}

GameObject* CourseworkGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass, bool isSpring) {
	GameObject* sphere = new GameObject();
	sphere->SetName("Sphere");

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->SetFriction(0.4f);
	sphere->GetPhysicsObject()->InitSphereInertia();
	if (isSpring) sphere->GetPhysicsObject()->SetCollisionType(CollisionType::Spring);

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* CourseworkGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass, bool isSpring) {
	GameObject* capsule = new GameObject();
	capsule->SetName("Capsule");

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius* 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();
	if (isSpring) capsule->GetPhysicsObject()->SetCollisionType(CollisionType::Spring);

	world->AddGameObject(capsule);

	return capsule;
}

GameObject* CourseworkGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass, bool isSpring) {
	GameObject* cube = new GameObject();
	cube->SetName("Cube");

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();
	if (isSpring) cube->GetPhysicsObject()->SetCollisionType(CollisionType::Spring);

	world->AddGameObject(cube);

	return cube;
}

bool CourseworkGame::SelectObject() {
	if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
		if (selectionObject) {	//set colour to deselected;
			selectionObject->GetRenderObject()->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
			selectionObject = nullptr;
			lockedObject	= nullptr;
		}
		/* Unhighlighting for functionality to fire forward collision
		if (forwardObject) {	//set colour to deselected;
			forwardObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
			forwardObject = nullptr;
			lockedObject	= nullptr;
		}
		*/

		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			selectionObject = (GameObject*)closestCollision.node;
			selectionObject->GetRenderObject()->SetColour(Vector4(0.0f, 1.0f, 0.0f, 1.0f));

			// Functionality to fire forward collision
			/*
			Ray forwardRay(selectionObject->GetTransform().GetPosition(), selectionObject->GetTransform().GetForwardFacing());
			RayCollision forwardCollision;
			if (world->Raycast(forwardRay, forwardCollision, true, selectionObject)) {
				forwardObject = (GameObject*)forwardCollision.node;
				forwardObject->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
				Debug::DrawLine(selectionObject->GetTransform().GetPosition(), forwardObject->GetTransform().GetPosition(),
					Vector4(0, 0, 1, 1), 1000.0f);
			}
			*/

			return true;
		}
		else {
			return false;
		}
	}

	if (selectionObject) {
		// Displaying debug infornation for selected object
		renderer->DrawString("Debug info for " + selectionObject->GetName() + ", World ID: " + std::to_string(selectionObject->GetWorldID()), Vector2(1.0f, 3.0f));
		renderer->DrawString("POSITION", Vector2(1.0f, 6.0f));
		renderer->DrawString("x: " + std::to_string(selectionObject->GetTransform().GetPosition().x), Vector2(1.0f, 9.0f));
		renderer->DrawString("y: " + std::to_string(selectionObject->GetTransform().GetPosition().y), Vector2(1.0f, 12.0f));
		renderer->DrawString("z: " + std::to_string(selectionObject->GetTransform().GetPosition().z), Vector2(1.0f, 15.0f));
		renderer->DrawString("ROTATION", Vector2(30.0f, 6.0f));
		renderer->DrawString("x: " + std::to_string(selectionObject->GetTransform().GetOrientation().ToEuler().x), Vector2(30.0f, 9.0f));
		renderer->DrawString("y: " + std::to_string(selectionObject->GetTransform().GetOrientation().ToEuler().y), Vector2(30.0f, 12.0f));
		renderer->DrawString("z: " + std::to_string(selectionObject->GetTransform().GetOrientation().ToEuler().z), Vector2(30.0f, 15.0f));
		renderer->DrawString("SCALE", Vector2(60.0f, 6.0f));
		renderer->DrawString("x: " + std::to_string(selectionObject->GetTransform().GetScale().x), Vector2(60.0f, 9.0f));
		renderer->DrawString("y: " + std::to_string(selectionObject->GetTransform().GetScale().y), Vector2(60.0f, 12.0f));
		renderer->DrawString("z: " + std::to_string(selectionObject->GetTransform().GetScale().z), Vector2(60.0f, 15.0f));

		renderer->DrawString("LIN. VELOCITY: ", Vector2(1.0f, 18.0f));
		renderer->DrawString("x: " + std::to_string(selectionObject->GetPhysicsObject()->GetLinearVelocity().x), Vector2(1.0f, 21.0f));
		renderer->DrawString("y: " + std::to_string(selectionObject->GetPhysicsObject()->GetLinearVelocity().x), Vector2(1.0f, 24.0f));
		renderer->DrawString("z: " + std::to_string(selectionObject->GetPhysicsObject()->GetLinearVelocity().x), Vector2(1.0f, 27.0f));
		
		renderer->DrawString("ANG. VELOCITY: ", Vector2(30.0f, 18.0f));
		renderer->DrawString("x: " + std::to_string(selectionObject->GetPhysicsObject()->GetAngularVelocity().x), Vector2(30.0f, 21.0f));
		renderer->DrawString("y: " + std::to_string(selectionObject->GetPhysicsObject()->GetAngularVelocity().x), Vector2(30.0f, 24.0f));
		renderer->DrawString("z: " + std::to_string(selectionObject->GetPhysicsObject()->GetAngularVelocity().x), Vector2(30.0f, 27.0f));

		float mass = 1 / selectionObject->GetPhysicsObject()->GetInverseMass();
		(mass < std::numeric_limits<float>::infinity()) ? renderer->DrawString("Mass: " + std::to_string(mass), Vector2(60.0f, 18.0f)) : renderer->DrawString("Mass: Infinite (0)", Vector2(60.0f, 18.0f));
	}

	if (lockedObject) {
		renderer->DrawString("Press L to unlock object!", Vector2(5.0f, 80.0f));
	}

	else if(selectionObject){
		renderer->DrawString("Press L to lock selected object object!", Vector2(5.0f, 80.0f));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject) {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}
	}

	return false;
}

void CourseworkGame::MainMenu(const std::string& title, const Vector4& colour) {
	PushdownMachine machine(new MainMenuScreen(this, renderer));

	while (window->UpdateWindow()) {
		float dt = window->GetTimer()->GetTimeDeltaSeconds();
		renderer->DrawString(title, Vector2(8.0f, 20.0f), colour, 30.0f);
		if (!machine.Update(dt)) {
			return;
		}
		Debug::FlushRenderables(dt);
		renderer->Render();
	}
}

void CourseworkGame::GenerateAIBehaviour() {
	BehaviourAction* lookForPlayer = new BehaviourAction("Go To Player", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			if (coins.size() > 0) {
				PowerupDistance p;

				for (auto& i : coins) {
					float newPowerupPos = Vector3::Dot(enemySphere->GetTransform().GetPosition(), i->GetTransform().GetPosition());
					if (newPowerupPos < p.distance) {
						p = PowerupDistance(p.gameObj, p.distance);
					}
				}

				// if distance to the closest power up < distance to the player
				if (p.distance < Vector3::Dot(enemySphere->GetTransform().GetPosition(), playerSphere->GetTransform().GetPosition())) {
					aiTarget = p.gameObj;
					return Failure;
				}
				else {
					aiTarget = playerSphere;
				}
			}
			else {
				aiTarget = playerSphere;
			}
			state = Ongoing;
		}
		if (state == Ongoing) {
			Pathfinding(aiTarget->GetTransform().GetPosition(), enemySphere->GetTransform().GetPosition());
			
			if (!pathfindingNodes.empty()) {
				Vector3 direction = pathfindingNodes[0] - enemySphere->GetTransform().GetPosition();
				direction.Normalise();
				enemySphere->GetPhysicsObject()->ApplyLinearImpulse(direction);

				Vector3 aiPos = enemySphere->GetTransform().GetPosition();
				Vector3 aiTargetPos = aiTarget->GetTransform().GetPosition();

				float distance = sqrt(pow(aiPos.x - aiTargetPos.x, 2) + pow(aiPos.y - aiTargetPos.y, 2) + pow(aiPos.z - aiTargetPos.z, 2));

				if (abs(distance) <= 20.0f) {
					SetGameState(2); // respawn
					return Success;
				}
			}
		}
		return state; // will be ongoing until success
	});

	BehaviourAction* lookForCoin = new BehaviourAction("Go To coin", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			if (coins.size() == 0) {
				return Failure;
			}
			aiTarget = coins[(rand() % coins.size())];
			state = Ongoing;
		}
		else if (state == Ongoing) {
			Pathfinding(aiTarget->GetTransform().GetPosition(), enemySphere->GetTransform().GetPosition());
			
			if (!pathfindingNodes.empty()) {
				Vector3 direction = pathfindingNodes[0] - enemySphere->GetTransform().GetPosition();
				direction.Normalise();

				enemySphere->GetPhysicsObject()->ApplyLinearImpulse(direction);

				// Coin collection and score increase handled as a collision in GameObject::OnCollisionBegin

				return Ongoing;
			}
		}
		return state; // will be ongoing until success
	});

	BehaviourSelector* selection = new BehaviourSelector("Look for Selection");
	selection->AddChild(lookForPlayer);
	selection->AddChild(lookForCoin);

	rootSequence = new BehaviourSequence("Root Sequence");
	rootSequence->AddChild(selection);
}

void CourseworkGame::UpdateAIBehaviour(float dt) {
	if (!rootSequence) {
		return;
	}

	BehaviourState state = Ongoing;
	if (state == Ongoing) {
		state = rootSequence->Execute(dt);
	}
}