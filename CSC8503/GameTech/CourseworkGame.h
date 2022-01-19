#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/PushdownMachine.h"
#include "../CSC8503Common/PushdownState.h"
#include "../CSC8503Common/BehaviourAction.h"
#include "../CSC8503Common/BehaviourSequence.h"
#include "../CSC8503Common/BehaviourSelector.h"

namespace NCL {
	namespace CSC8503 {

		class CourseworkGame {
		public:
			CourseworkGame(Window* w);
			~CourseworkGame();

			virtual void UpdateGame(float dt);

			void SetGameState(int value);
			int GetCurrentLevel() { return currentLevel; }
		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void LoadLevel1();
			void LoadLevel2();

			void DisplayPathfinding();
			void Pathfinding(Vector3 startPos, Vector3 endPos);
			std::vector<Vector3> pathfindingNodes;
			GameObject* playerSphere;
			GameObject* enemySphere;

			// L1 Gameplay
			void UpdateWreckingBall();
			void L1Gameplay();

			// L2 Gameplay
			void L2Gameplay(float dt);
			void InitialiseWreckingBall();
	
			bool SelectObject();
			void LockedObjectMovement();

			GameObject* AddAABBFloorToWorld(const Vector3& position, const Vector3& floorSize, const Vector4& colour = Vector4(1,1,1,1), bool isSpring = false);
			GameObject* AddOBBFloorToWorld(const Vector3& position, const Vector3& rotation, const Vector3& floorSize, const Vector4& colour = Vector4(1,1,1,1), 
											bool isSpring = false, float elasticity = 0.8f, float friction = 0.8f);
			GameObject* AddBalancingPlatformToWorld(const Vector3& position, const Vector3& rotation, const Vector3& floorSize, const Vector4& colour = Vector4(1, 1, 1, 1),
											bool isSpring = false, float elasticity = 0.8f, float friction = 0.8f);
			GameObject* AddPistonPlatformToWorld(const Vector3& position, const Vector3& rotation, const Vector3& floorSize, const Vector3& pistonMovementConstraint, bool isSpring = false);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f, bool isSpring = false);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f, bool isSpring = false);
			GameObject* AddCoinToWorld(const Vector3& position);
			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass = 10.0f, bool isSpring = false);

			void MainMenu(const std::string& title = "", const Vector4& colour = Vector4(1,1,1,1));
			std::string GetTime();

			// AI
			void GenerateAIBehaviour();
			void UpdateAIBehaviour(float dt);
			BehaviourSequence* rootSequence;
			GameObject* aiTarget;

			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;
			GameObject* forwardObject = nullptr; // Tutorial 1 Q.2

			OGLMesh*	capsuleMesh = nullptr;
			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLMesh*	coinMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLShader*	basicShader = nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

		private:
			std::chrono::time_point<std::chrono::system_clock> gameTime;
			std::vector<GameObject*> pistonPlatforms;
			std::vector<GameObject*> coins;
			Window* window;
			
			int currentLevel = 0;

			GameObject* wreckingBallEnd;
			bool wreckingBallDirection = false;
		};

		class MainMenuScreen : public PushdownState {
		public:
			MainMenuScreen(CourseworkGame* tg, GameTechRenderer* r) {
				this->renderer = r;
				this->game = tg;
			}
			~MainMenuScreen() {
			}
			PushdownResult OnUpdate(float dt, PushdownState** newState) override {

				if (game->GetCurrentLevel() != 0) {
					// A game has been initialised, so provide option to resume
					minChoice = 0;
					maxChoice = 3;
					(currentChoice == 0) ? renderer->DrawString("Resume", Vector2(42.0f, 20.0f), Vector4(0, 0, 1, 1), 30.0f) :
						renderer->DrawString("Resume", Vector2(42.0f, 20.0f), Vector4(1, 1, 1, 1), 30.0f);
				}
				else {
					minChoice = 1;
					maxChoice = 3;
				}

				(currentChoice == 1) ? renderer->DrawString("Play Part 1", Vector2(35.0f, 40.0f), Vector4(0, 0, 1, 1), 30.0f) :
					renderer->DrawString("Play Part 1", Vector2(35.0f, 40.0f), Vector4(1, 1, 1, 1), 30.0f);

				(currentChoice == 2) ? renderer->DrawString("Play Part 2", Vector2(35.0f, 60.0f), Vector4(0, 0, 1, 1), 30.0f) :
					renderer->DrawString("Play Part 2", Vector2(35.0f, 60.0f), Vector4(1, 1, 1, 1), 30.0f);

				(currentChoice == 3) ? renderer->DrawString("Quit", Vector2(45.0f, 80.0f), Vector4(0, 0, 1, 1), 30.0f) :
					renderer->DrawString("Quit", Vector2(45.0f, 80.0f), Vector4(1, 1, 1, 1), 30.0f);

				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE) && game->GetCurrentLevel() != 0) {
					return PushdownResult::Pop;
				}
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::UP)) {
					currentChoice = (currentChoice > minChoice) ? (currentChoice - 1) : maxChoice;
				}
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::DOWN)) {
					currentChoice = (currentChoice < maxChoice) ? (currentChoice + 1) : minChoice;
				}
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::RETURN)) {
					if (currentChoice == 0 && game->GetCurrentLevel() != 0) {
						return PushdownResult::Pop;
					}
					game->SetGameState(currentChoice);
					return PushdownResult::Pop;
				}

				return PushdownResult::NoChange;
			};

			void OnAwake() override {
				// Deciding on which option the cursor starts, determined by whether the Resume option is being shown
				currentChoice = (game->GetCurrentLevel() == 0) ? 1 : 0;
			}

		private:
			GameTechRenderer* renderer;
			CourseworkGame* game;
			int currentChoice;
			int minChoice;
			int maxChoice;
		};
		struct PowerupDistance {
			PowerupDistance() {
				this->gameObj = nullptr;
				distance = FLT_MAX;
			}
			PowerupDistance(GameObject* g, float d) {
				this->gameObj = g;
				this->distance = d;
			}
			GameObject* gameObj;
			float distance;
		};
	}
}

