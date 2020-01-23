#pragma once
#include "Vec3.h"

struct Material {
	Vec3 Ka;		// Couleur ambiente
	Vec3 Kd;		// Couleur diffuse
	Vec3 Ks;		// Couleur sp�culaire
	int shininess;  // R�flections
};