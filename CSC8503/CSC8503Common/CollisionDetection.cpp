#include "CollisionDetection.h"
#include "CollisionVolume.h"
#include "AABBVolume.h"
#include "OBBVolume.h"
#include "SphereVolume.h"
#include "../../Common/Vector2.h"
#include "../../Common/Window.h"
#include "../../Common/Maths.h"
#include "../../Common/Plane.h"
#include "Debug.h"

#include <list>

using namespace NCL;

bool CollisionDetection::RayPlaneIntersection(const Ray&r, const Plane&p, RayCollision& collisions) {
	float ln = Vector3::Dot(p.GetNormal(), r.GetDirection());

	if (ln == 0.0f) {
		return false; //direction vectors are perpendicular!
	}
	
	Vector3 planePoint = p.GetPointOnPlane();

	Vector3 pointDir = planePoint - r.GetPosition();

	float d = Vector3::Dot(pointDir, p.GetNormal()) / ln;

	collisions.collidedAt = r.GetPosition() + (r.GetDirection() * d);

	return true;
}

bool CollisionDetection::RayIntersection(const Ray& r,GameObject& object, RayCollision& collision) {
	bool hasCollided = false;

	const Transform& worldTransform = object.GetTransform();
	const CollisionVolume* volume	= object.GetBoundingVolume();

	if (!volume) {
		return false;
	}

	switch (volume->type) {
		case VolumeType::AABB:		hasCollided = RayAABBIntersection(r, worldTransform, (const AABBVolume&)*volume	, collision); break;
		case VolumeType::OBB:		hasCollided = RayOBBIntersection(r, worldTransform, (const OBBVolume&)*volume	, collision); break;
		case VolumeType::Sphere:	hasCollided = RaySphereIntersection(r, worldTransform, (const SphereVolume&)*volume	, collision); break;
		case VolumeType::Capsule:	hasCollided = RayCapsuleIntersection(r, worldTransform, (const CapsuleVolume&)*volume, collision); break;
	}

	return hasCollided;
}

//	Raybox can be used for both AABB and OBB
bool CollisionDetection::RayBoxIntersection(const Ray&r, const Vector3& boxPos, const Vector3& boxSize, RayCollision& collision) {
	Vector3 boxMin = boxPos - boxSize;
	Vector3 boxMax = boxPos + boxSize;
	
	Vector3 rayPos = r.GetPosition();
	Vector3 rayDir = r.GetDirection();
	
	Vector3 tVals(-1, -1, -1);
	
	for (int i = 0; i < 3; ++i) { // get best 3 intersections
		if (rayDir[i] > 0) {
			tVals[i] = (boxMin[i] - rayPos[i]) / rayDir[i];
		}
		else if (rayDir[i] < 0) {
			tVals[i] = (boxMax[i] - rayPos[i]) / rayDir[i];
		}	
	}
	float bestT = tVals.GetMaxElement();
	if (bestT < 0.0f) {
		return false; // no backwards rays !
	}

	Vector3 intersection = rayPos + (rayDir * bestT);
	const float epsilon = 0.0001f; // an amount of leeway in our calcs
	for (int i = 0; i < 3; ++i) { // iterate over each access, working out whether the intersection point is too far to one side or the other
		if (intersection[i] + epsilon < boxMin[i] ||
			intersection[i] - epsilon > boxMax[i]) {
			return false; // best intersection doesn?t touch the box !
		}
	}
	collision.collidedAt = intersection;
	collision.rayDistance = bestT;
	
	return true;
}

bool CollisionDetection::RayAABBIntersection(const Ray&r, const Transform& worldTransform, const AABBVolume& volume, RayCollision& collision) {
	Vector3 boxPos = worldTransform.GetPosition();
	Vector3 boxSize = volume.GetHalfDimensions();
	return RayBoxIntersection(r, boxPos, boxSize, collision);
}

bool CollisionDetection::RayOBBIntersection(const Ray&r, const Transform& worldTransform, const OBBVolume& volume, RayCollision& collision) {
	Quaternion orientation = worldTransform.GetOrientation();
	Vector3 position = worldTransform.GetPosition();
	
	Matrix3 transform = Matrix3(orientation);
	Matrix3 invTransform = Matrix3(orientation.Conjugate());
	
	Vector3 localRayPos = r.GetPosition() - position;
	
	Ray tempRay(invTransform * localRayPos, invTransform * r.GetDirection());
	
	bool collided = RayBoxIntersection(tempRay, Vector3(), volume.GetHalfDimensions(), collision);
	
	if (collided) {
		collision.collidedAt = transform * collision.collidedAt + position;
	}
	
	return collided;
}

// Using Ray-Plane to get some coordinates for where the ray has hit the capsule - then place a sphere and do Ray-Sphere on it
bool CollisionDetection::RayCapsuleIntersection(const Ray& r, const Transform& worldTransform, const CapsuleVolume& volume, RayCollision& collision) {
	Vector3 firstPoint = worldTransform.GetPosition() + (worldTransform.GetOrientation() * (Vector3(0, 1, 0) * (volume.GetHalfHeight() - volume.GetRadius())));
	Vector3 secondPoint = worldTransform.GetPosition() - (worldTransform.GetOrientation() * (Vector3(0, 1, 0) * (volume.GetHalfHeight() - volume.GetRadius())));

	// Cross product between the capsules center and the normal
	Vector3 c = Vector3::Cross(worldTransform.GetPosition(), r.GetPosition() - worldTransform.GetPosition());
	Vector3 thirdPoint = worldTransform.GetPosition() + c.Normalised();

	Plane p = Plane::PlaneFromTri(firstPoint, secondPoint, thirdPoint);
	if (!RayPlaneIntersection(r, p, collision)) return false;

	Vector3 capsuleDirection = firstPoint - secondPoint;
	float capsuleLength = capsuleDirection.Length();
	capsuleDirection.Normalise();

	Vector3 rayCapDirection = collision.collidedAt - secondPoint;
	float dotProd = Maths::Clamp(Vector3::Dot(rayCapDirection, capsuleDirection), 0.0f, capsuleLength);

	// Create sphere for Ray-Sphere check
	SphereVolume sphere(volume.GetRadius());
	Transform sphereTransform;
	sphereTransform.SetPosition(secondPoint + (capsuleDirection * dotProd));
	sphereTransform.SetScale(Vector3(volume.GetRadius(), volume.GetRadius(), volume.GetRadius()));
	return RaySphereIntersection(r, sphereTransform, sphere, collision);
}

bool CollisionDetection::RaySphereIntersection(const Ray&r, const Transform& worldTransform, const SphereVolume& volume, RayCollision& collision) {
	Vector3 spherePos = worldTransform.GetPosition();
	float sphereRadius = volume.GetRadius();
		
	// Get the direction between the ray origin and the sphere origin
	Vector3 dir = (spherePos - r.GetPosition());
		
	// Then project the sphere ?s origin onto our ray direction vector
	float sphereProj = Vector3::Dot(dir, r.GetDirection());
		
	if (sphereProj < 0.0f) {
		return false; // point is behind the ray!
	}
		
	// Get closest point on ray line to sphere
	Vector3 point = r.GetPosition() + (r.GetDirection() * sphereProj);
		
	float sphereDist = (point - spherePos).Length();
		
	if (sphereDist > sphereRadius) {
		return false;
	}
		
	float offset = sqrt((sphereRadius * sphereRadius) - (sphereDist * sphereDist));
		
	collision.rayDistance = sphereProj - (offset);
	collision.collidedAt = r.GetPosition() + (r.GetDirection() * collision.rayDistance);
		
	return true;
}

Matrix4 GenerateInverseView(const Camera &c) {
	float pitch = c.GetPitch();
	float yaw	= c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(-yaw, Vector3(0, -1, 0)) *
		Matrix4::Rotation(-pitch, Vector3(-1, 0, 0));

	return iview;
}

Vector3 CollisionDetection::Unproject(const Vector3& screenPos, const Camera& cam) {
	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	float aspect	= screenSize.x / screenSize.y;
	float fov		= cam.GetFieldOfVision();
	float nearPlane = cam.GetNearPlane();
	float farPlane  = cam.GetFarPlane();

	/*	Create our inverted matrix! Note how that to get a correct inverse matrix,
		the order of matrices used to form it are inverted, too. */
	Matrix4 invVP = GenerateInverseView(cam) * GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

	/*	Our mouse position x and y values are in 0 to screen dimensions range,
		so we need to turn them into the -1 to 1 axis range of clip space.
		We can do that by dividing the mouse values by the width and height of the
		screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
		and then subtracting 1 (-1.0 to 1.0). */
	Vector4 clipSpace = Vector4(
		(screenPos.x / (float)screenSize.x) * 2.0f - 1.0f,
		(screenPos.y / (float)screenSize.y) * 2.0f - 1.0f,
		(screenPos.z),
		1.0f
	);

	//	Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	/*	our transformed w coordinate is now the 'inverse' perspective divide, so
		we can reconstruct the final world space by dividing x,y,and z by w. */
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

Ray CollisionDetection::BuildRayFromMouse(const Camera& cam) {
	Vector2 screenMouse = Window::GetMouse()->GetAbsolutePosition();
	Vector2 screenSize	= Window::GetWindow()->GetScreenSize();

	/*	We remove the y axis mouse position from height as OpenGL is 'upside down',
		and thinks the bottom left is the origin, instead of the top left! */
	Vector3 nearPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		-0.99999f
	);

	/*	We also don't use exactly 1.0 (the normalised 'end' of the far plane) as this
		causes the unproject function to go a bit weird. */
	Vector3 farPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		0.99999f
	);

	Vector3 a = Unproject(nearPos, cam);
	Vector3 b = Unproject(farPos, cam);
	Vector3 c = b - a;

	c.Normalise();

	return Ray(cam.GetPosition(), c);
}

//http://bookofhook.com/mousepick.pdf
Matrix4 CollisionDetection::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	Matrix4 m;

	float t = tan(fov*PI_OVER_360);

	float neg_depth = nearPlane - farPlane;

	const float h = 1.0f / t;

	float c = (farPlane + nearPlane) / neg_depth;
	float e = -1.0f;
	float d = 2.0f*(nearPlane*farPlane) / neg_depth;

	m.array[0]  = aspect / h;
	m.array[5]  = tan(fov*PI_OVER_360);

	m.array[10] = 0.0f;
	m.array[11] = 1.0f / d;

	m.array[14] = 1.0f / e;

	m.array[15] = -c / (d*e);

	return m;
}

// Generating an inverse view matrix, pretty much an exact inversion of the BuildViewMatrix function of the Camera class
Matrix4 CollisionDetection::GenerateInverseView(const Camera &c) {
	float pitch = c.GetPitch();
	float yaw	= c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(yaw, Vector3(0, 1, 0)) *
		Matrix4::Rotation(pitch, Vector3(1, 0, 0));

	return iview;
}


/*
	This function takes a 2D position, such as the mouse position, and 'unprojects' it, 
	to generate a 3D world space position for it. We can turn a clip space
	position back to a 3D position by multiply it by the INVERSE of the
	view projection matrix (the model matrix has already been assumed to have
	'transformed' the 2D point). So, to form the inverted matrix, we need the aspect 
	and fov used to create the projection matrix of our scene, and the camera used to 
	form the view matrix.
*/
Vector3	CollisionDetection::UnprojectScreenPosition(Vector3 position, float aspect, float fov, const Camera &c) {
	/*	Create our inverted matrix! Note how that to get a correct inverse matrix,
		the order of matrices used to form it are inverted, too. */
	Matrix4 invVP = GenerateInverseView(c) * GenerateInverseProjection(aspect, fov, c.GetNearPlane(), c.GetFarPlane());

	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	/*	Our mouse position x and y values are in 0 to screen dimensions range,
		so we need to turn them into the -1 to 1 axis range of clip space.
		We can do that by dividing the mouse values by the width and height of the
		screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
		and then subtracting 1 (-1.0 to 1.0). */
	Vector4 clipSpace = Vector4(
		(position.x / (float)screenSize.x) * 2.0f - 1.0f,
		(position.y / (float)screenSize.y) * 2.0f - 1.0f,
		(position.z) - 1.0f,
		1.0f
	);

	//	Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	/*	Our transformed w coordinate is now the 'inverse' perspective divide, so
		we can reconstruct the final world space by dividing x,y,and z by w. */
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

bool CollisionDetection::ObjectIntersection(GameObject* a, GameObject* b, CollisionInfo& collisionInfo) {
	const CollisionVolume* volA = a->GetBoundingVolume();
	const CollisionVolume* volB = b->GetBoundingVolume();

	if (!volA || !volB) {
		return false;
	}

	collisionInfo.a = a;
	collisionInfo.b = b;

	Transform& transformA = a->GetTransform();
	Transform& transformB = b->GetTransform();

	VolumeType pairType = (VolumeType)((int)volA->type | (int)volB->type);

	if (pairType == VolumeType::AABB) {
		return AABBIntersection((AABBVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}

	if (pairType == VolumeType::Sphere) {
		return SphereIntersection((SphereVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}

	if (pairType == VolumeType::OBB) {
		return OBBIntersection((OBBVolume&)*volA, transformA, (OBBVolume&)*volB, transformB, collisionInfo);
	}
	
	if (pairType == VolumeType::Capsule) {
		return CapsuleIntersection((CapsuleVolume&)*volA, transformA, (CapsuleVolume&)*volB, transformB, collisionInfo);
	}

	if (volA->type == VolumeType::AABB && volB->type == VolumeType::Sphere) {
		return AABBSphereIntersection((AABBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBSphereIntersection((AABBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::Sphere) {
		return SphereCapsuleIntersection((CapsuleVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::Capsule) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return SphereCapsuleIntersection((CapsuleVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}
	
	if (volA->type == VolumeType::AABB && volB->type == VolumeType::Capsule) {
		return AABBCapsuleIntersection((AABBVolume&)*volA, transformA, (CapsuleVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBCapsuleIntersection((AABBVolume&)*volB, transformB, (CapsuleVolume&)*volA, transformA, collisionInfo);
	}
	
	if (volA->type == VolumeType::OBB && volB->type == VolumeType::Sphere) {
		return OBBSphereIntersection((OBBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::OBB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return OBBSphereIntersection((OBBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}
	
	if (volA->type == VolumeType::OBB && volB->type == VolumeType::Capsule) {
		return OBBCapsuleIntersection((OBBVolume&)*volA, transformA, (CapsuleVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::OBB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return OBBCapsuleIntersection((OBBVolume&)*volB, transformB, (CapsuleVolume&)*volA, transformA, collisionInfo);
	}

	return false;
}

bool CollisionDetection::AABBTest(const Vector3& posA, const Vector3& posB, const Vector3& halfSizeA, const Vector3& halfSizeB) {
	Vector3 delta = posB - posA;
	Vector3 totalSize = halfSizeA + halfSizeB;
	
	if (abs(delta.x) < totalSize.x && abs(delta.y) < totalSize.y && abs(delta.z) < totalSize.z) {
		return true;
	}
	
	return false;
}

//AABB/AABB Collisions
bool CollisionDetection::AABBIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxAPos = worldTransformA.GetPosition();
	Vector3 boxBPos = worldTransformB.GetPosition();
	
	Vector3 boxASize = volumeA.GetHalfDimensions();
	Vector3 boxBSize = volumeB.GetHalfDimensions();
	
	bool overlap = AABBTest(boxAPos, boxBPos, boxASize, boxBSize);

	if (overlap) {
		static const Vector3 faces[6] = {
			Vector3(-1, 0, 0), Vector3(1, 0, 0),
			Vector3(0, -1, 0), Vector3(0, 1, 0),
			Vector3(0, 0, -1), Vector3(0, 0, 1),
		};
		
		Vector3 maxA = boxAPos + boxASize;
		Vector3 minA = boxAPos - boxASize;
		
		Vector3 maxB = boxBPos + boxBSize;
		Vector3 minB = boxBPos - boxBSize;
		
		float distances[6] = {
			(maxB.x - minA.x),// distance of box ?b? to ?left? of ?a?.
			(maxA.x - minB.x),// distance of box ?b? to ?right? of ?a?.
			(maxB.y - minA.y),// distance of box ?b? to ?bottom? of ?a?.
			(maxA.y - minB.y),// distance of box ?b? to ?top? of ?a?.
			(maxB.z - minA.z),// distance of box ?b? to ?far? of ?a?.
			(maxA.z - minB.z) // distance of box ?b? to ?near? of ?a?.
		};
		
		float penetration = FLT_MAX;
		Vector3 bestAxis;
		
		for (int i = 0; i < 6; i++)
		{
			if (distances[i] < penetration) {
				penetration = distances[i];
				bestAxis = faces[i];
			}
		}
		
		collisionInfo.AddContactPoint(Vector3(), Vector3(), bestAxis, penetration);
		return true;
	}
	
	return false;
}

//Sphere / Sphere Collision
bool CollisionDetection::SphereIntersection(const SphereVolume& volumeA, const Transform& worldTransformA,
											const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	float radii = volumeA.GetRadius() + volumeB.GetRadius();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();
		
	float deltaLength = delta.Length();
		
	if (deltaLength < radii) {
		float penetration = (radii - deltaLength);
		Vector3 normal = delta.Normalised();
		Vector3 localA = normal * volumeA.GetRadius();
		Vector3 localB = -normal * volumeB.GetRadius();
			
		collisionInfo.AddContactPoint(localA, localB, normal, penetration);
		return true; // colliding
	}
		
	return false;
}

//AABB - Sphere Collision
bool CollisionDetection::AABBSphereIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	Vector3 boxSize = volumeA.GetHalfDimensions();
	
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	Vector3 closestPointOnBox = Maths::Clamp(delta, -boxSize, boxSize);

	Vector3 localPoint = delta - closestPointOnBox;
	float distance = localPoint.Length();
	
	if (distance < volumeB.GetRadius()) {// yes, we ?re colliding
		Vector3 collisionNormal = localPoint.Normalised();
		float penetration = (volumeB.GetRadius() - distance);
		
		Vector3 localA = Vector3();
		Vector3 localB = -collisionNormal * volumeB.GetRadius();
		
		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
		return true;
	}
	
	return false;
}

bool CollisionDetection::CapsuleIntersection(	const CapsuleVolume& volumeA, const Transform& worldTransformA,
												const CapsuleVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	// Capsule A
	Vector3 capsuleTopA = worldTransformA.GetPosition() + (worldTransformA.GetOrientation() * (Vector3(0, 1, 0) * (volumeA.GetHalfHeight() - volumeA.GetRadius())));
	Vector3 capsuleBottomA = worldTransformA.GetPosition() - (worldTransformA.GetOrientation() * (Vector3(0, 1, 0) * (volumeA.GetHalfHeight() - volumeA.GetRadius())));
	Vector3 capsuleDirectionA = capsuleTopA - capsuleBottomA;
	float capsuleLengthA = capsuleDirectionA.Length();
	capsuleDirectionA.Normalise();

	// Capsule B
	Vector3 capsuleTopB = worldTransformB.GetPosition() + (worldTransformB.GetOrientation() * (Vector3(0, 1, 0) * (volumeB.GetHalfHeight() - volumeB.GetRadius())));
	Vector3 capsuleBottomB = worldTransformB.GetPosition() - (worldTransformB.GetOrientation() * (Vector3(0, 1, 0) * (volumeB.GetHalfHeight() - volumeB.GetRadius())));
	Vector3 capsuleDirectionB = capsuleTopB - capsuleBottomB;
	float capsuleLengthB = capsuleDirectionB.Length();
	capsuleDirectionB.Normalise();

	Vector3 v0 = capsuleTopB - capsuleTopA;
	Vector3 v1 = capsuleBottomB - capsuleTopA;
	Vector3 v2 = capsuleTopB - capsuleBottomA;
	Vector3 v3 = capsuleBottomB - capsuleBottomA;

	float d0 = Vector3::Dot(v0, v0);
	float d1 = Vector3::Dot(v1, v1);
	float d2 = Vector3::Dot(v2, v2);
	float d3 = Vector3::Dot(v3, v3);

	Vector3 bestA;
	if (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1) {
		bestA = capsuleBottomA;
	}
	else {
		bestA = capsuleTopA;
	}

	return false;
}

// Sphere-Capsule by Sphere-Sphere
bool CollisionDetection::SphereCapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 capsuleTop = worldTransformA.GetPosition() + (worldTransformA.GetOrientation() * (Vector3(0, 1, 0) * (volumeA.GetHalfHeight() - volumeA.GetRadius())));
	Vector3 capsuleBottom = worldTransformA.GetPosition() - (worldTransformA.GetOrientation() * (Vector3(0, 1, 0) * (volumeA.GetHalfHeight() - volumeA.GetRadius())));

	Vector3 capsuleDirection = capsuleTop - capsuleBottom; 
	float capsuleLength = capsuleDirection.Length();
	capsuleDirection.Normalise();

	Vector3 rayCapDirection = worldTransformB.GetPosition() - capsuleBottom;
	float dotProd = Maths::Clamp(Vector3::Dot(rayCapDirection, capsuleDirection), 0.0f, capsuleLength);

	// Create sphere for Ray-Sphere check
	SphereVolume sphereFromCapsule(volumeA.GetRadius());
	Transform sphereFromCapsuleTransform;
	sphereFromCapsuleTransform.SetPosition(capsuleBottom + (capsuleDirection * dotProd));
	sphereFromCapsuleTransform.SetScale(Vector3(volumeA.GetRadius(), volumeA.GetRadius(), volumeA.GetRadius()));
	return SphereIntersection(sphereFromCapsule, sphereFromCapsuleTransform, volumeB, worldTransformB, collisionInfo);
}

// AABB-Capsule by AABB-Sphere
bool CollisionDetection::AABBCapsuleIntersection(
	const AABBVolume& volumeA, const Transform& worldTransformA,
	const CapsuleVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	Vector3 capsuleTop = worldTransformB.GetPosition() + (worldTransformB.GetOrientation() * (Vector3(0, 1, 0) * (volumeB.GetHalfHeight() - volumeB.GetRadius())));
	Vector3 capsuleBottom = worldTransformB.GetPosition() - (worldTransformB.GetOrientation() * (Vector3(0, 1, 0) * (volumeB.GetHalfHeight() - volumeB.GetRadius())));

	Vector3 capsuleDirection = capsuleTop - capsuleBottom;
	float capsuleLength = capsuleDirection.Length();
	capsuleDirection.Normalise();

	Vector3 rayCapDirection = worldTransformB.GetPosition() - capsuleBottom;
	float dotProd = Maths::Clamp(Vector3::Dot(rayCapDirection, capsuleDirection), 0.0f, capsuleLength);

	// Create sphere for Ray-Sphere check
	SphereVolume sphereFromCapsule(volumeB.GetRadius());
	Transform sphereFromCapsuleTransform;
	sphereFromCapsuleTransform.SetPosition(capsuleBottom + (capsuleDirection * dotProd));
	sphereFromCapsuleTransform.SetScale(Vector3(volumeB.GetRadius(), volumeB.GetRadius(), volumeB.GetRadius()));

	return AABBSphereIntersection(volumeA, worldTransformA, sphereFromCapsule, sphereFromCapsuleTransform, collisionInfo);
}

bool CollisionDetection::OBBIntersection(
	const OBBVolume& volumeA, const Transform& worldTransformA,
	const OBBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	return false;
}

bool CollisionDetection::OBBSphereIntersection(
	const OBBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	// Inverse orientation of the OBB
	Quaternion x = worldTransformA.GetOrientation();
	x = x.Conjugate();

	// Transform the sphere into the relative coordinates of the box using this inverse orientation
	Vector3 localPosRelative = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	localPosRelative = x * localPosRelative;

	Transform localSphere;
	localSphere.SetPosition(localPosRelative);

	AABBVolume aabb(volumeA.GetHalfDimensions());
	Transform aabbTransform;
	aabbTransform.SetPosition(localPosRelative);
	aabbTransform.SetScale(worldTransformA.GetScale());

	Vector3 boxSize = aabb.GetHalfDimensions();

	Vector3 closestPointOnBox = Maths::Clamp(localPosRelative, -boxSize, boxSize);

	float distance = (closestPointOnBox - localPosRelative).Length();

	if (distance < volumeB.GetRadius()) { // we're colliding
		Vector3 collisionNormal = worldTransformA.GetOrientation() * -(closestPointOnBox - localPosRelative).Normalised();
		float penetration = (volumeB.GetRadius() - distance);

		Vector3 localA = (worldTransformA.GetOrientation() * closestPointOnBox) + worldTransformA.GetPosition();
		Vector3 localB = -collisionNormal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
		return true;
	}

	return false;
}

bool CollisionDetection::OBBCapsuleIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
												const CapsuleVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	// Get a sphere from Capsule, then OBB-Sphere
	return false;
}