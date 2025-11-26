
IF(BUILD_BULLET3)
	SUBDIRS(  Bullet3Dynamics Bullet3Collision Bullet3Geometry )
ENDIF(BUILD_BULLET3)


SUBDIRS(   BulletInverseDynamics BulletSoftBody BulletCollision BulletDynamics LinearMath Bullet3Common)
