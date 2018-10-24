#include "demo2.h"
#include "Ray.h"
#include "Object.h"

//window resolution (default 640x480)
int windowX = 640;
int windowY = 480;

//Camera matrices
//For raytracer, only need camera transformation (View Matrix)
glm::mat4 viewMatrix; //The transformation matrix for the virtual camera
glm::vec3 originP = glm::vec3(0.0f, 0.0f, 200.0f);
//float changer = 10.0f;


// A container for all the objects in the scene
vector<unique_ptr<Object>> objects;

// The control panel variables - see below
glm::vec3 lightPos;
bool activateShadows;
bool activatePhong;
bool activateReflections;
int maxReflections;
//bool activateMovingCameraTest;
int scene;

//Perform any cleanup of resources here
void cleanup()
{
	objects.clear();
}



//Forward declaration of functions, see below for more information
bool CheckIntersection(const Ray &ray, IntersectInfo &info);
float CastRay(Ray &ray, Payload &payload);

//Function for testing for intersection with all the objects in the scene
//If an object is hit then info contains the information on the intersection,
//returns true if an object is hit, false otherwise
bool CheckIntersection(const Ray &ray, IntersectInfo &info)
{
	bool found = false;
	// For each object in the scene
	for (auto obj = objects.begin(); obj != objects.end(); ++obj) {
		IntersectInfo tempInfo;
		// Check if the ray intersects the object
		if ((*obj)->Intersect(ray, tempInfo)) {
			if (!found) {
				found = true;
				info = tempInfo;
			}
			else {
				// Remember only the earliest collision
				if (tempInfo.time < info.time) {
					info = tempInfo;
				}
			}
		}
	}
	return found;
}

//Recursive ray-casting function
//Called for each pixel and each time a ray is reflected/used for shadow testing
//@ray The ray we are casting
//@payload Information on the current ray i.e. the cumulative color and the number of bounces it has performed
//returns either the time of intersection with an object (the coefficient t in the equation: RayPosition = RayOrigin + t*RayDirection) or zero to indicate no intersection
float CastRay(Ray &ray, Payload &payload)
{
	//Check if the ray intersects something
	IntersectInfo info;
	if (CheckIntersection(ray, info)) {

		// Move the collision point slightly up the normal to avoid shadow ray colliding again
		glm::vec3 hitPoint_fix = info.hitPoint + (0.1f * info.normal);

		// Initialize a normal vector pointing towards the light source
		// PL = L - P
		glm::vec3 lightVec = glm::normalize(glm::vec3(lightPos - hitPoint_fix));

		if (activateShadows) {

			Ray shadowRay( 	hitPoint_fix, 	//The origin of the ray we are casting
			                lightVec		//The direction the ray is travelling in
			             );
			IntersectInfo shadowInfo;
			// Cast the shadow ray
			bool shadowed = CheckIntersection(shadowRay, shadowInfo);

			float collisionDist = glm::distance(hitPoint_fix, shadowInfo.hitPoint);
			float lightDist = glm::distance(hitPoint_fix, lightPos);
			if (shadowed) {
				if (collisionDist < lightDist) {
					payload.shadowed = true;
				}
				else {
					payload.shadowed = false;
				}
			}
			else {
				payload.shadowed = false;
			}
		}

		if (activatePhong) {
			// Compute Phong illumination
			glm::vec3 normOut = info.normal;
			if (glm::dot(lightVec, normOut) < 0) {
				normOut = -1.0f * normOut;
			}
			glm::vec3 eyeVec = glm::normalize(ray.origin - hitPoint_fix);
			// Intensity constants
			float attenuationFactor = 0.0;
			float light_source_intensity = 1.0;
			float specular_intensity = info.material->specularExponent;

			// Reflectivity constants
			float diffuse_reflectivity = 0.6;
			float specular_reflectivity = 0.8;

			// Ambient lighting constant
			float ambient_lighting = 0.1;

			// Dot product : a . b = |a||b|cosø
			// |lightVec| == |normOut| == 1
			// So lightVec . normOut = cosø
			float cosine_theta = glm::max(0.0f, abs(glm::dot(lightVec, normOut)));

			// cos(a) = (2N(L.N)-L).V
			float cosine_alpha = glm::max(0.0f, glm::dot(((2.0f * normOut * (glm::dot(lightVec, normOut))) - lightVec), eyeVec));

			// Using the equations in the coursework pdf
			float diff    = light_source_intensity * diffuse_reflectivity * cosine_theta;
			float spec    = light_source_intensity * specular_reflectivity * pow(cosine_alpha, specular_intensity);
			float ambient = ambient_lighting;
			float all = (diff + spec + ambient);

			glm::vec3 diffMat = info.material->diffuse;
			glm::vec3 specMat = info.material->specular;
			glm::vec3 ambMat = info.material->ambient;

			// Only use the ambient lighting if the pixel is in shadow
			if (payload.shadowed) {
				diff = 0;
				spec = 0;
			}

			// Calculate the RGB colour using the Material
			float red_free = (diff * diffMat.x) + (spec * specMat.x) + (ambient * ambMat.x);
			float green_free = (diff * diffMat.y) + (spec * specMat.y) + (ambient * ambMat.y);
			float blue_free = (diff * diffMat.z) + (spec * specMat.z) + (ambient * ambMat.z);
			
			// Constrain the colour floats to [0,1]
			float red   = glm::max(0.0f, glm::min(1.0f, red_free));
			float green = glm::max(0.0f, glm::min(1.0f, green_free));
			float blue  = glm::max(0.0f, glm::min(1.0f, blue_free));

			//Toggles Phong on and off
			payload.color = info.material->Klocal * glm::vec3(red, green, blue);
		}
		else {
			if (activateShadows) {
				if (payload.shadowed) {
					payload.color = glm::vec3(0.0f);
				}
				else {
					payload.color = info.material->Klocal * info.material->ambient;
				}
			}
			else {
				// No shadows and no Phong so just colour everything by its ambient colour
				payload.color = info.material->Klocal * info.material->ambient;
			}
		}

		// The recursive reflection rays - adapted from the lecture slides
		if (activateReflections) {
			payload.numBounces++;

			if (info.material->Kreflectivity > 0 && payload.numBounces <= maxReflections) {
				IntersectInfo bounceInfo;
				// r = i - 2N(i.n)
				glm::vec3 reflDir = glm::normalize(ray.direction - 2.0f * info.normal * (glm::dot(ray.direction, info.normal)));
				Ray reflectionRay( 	hitPoint_fix, 	//The origin of the ray we are casting
				                    reflDir		//The direction the ray is travelling in
				                 );
				Payload refPayload;
				refPayload.numBounces = payload.numBounces;
				CastRay(reflectionRay, refPayload);
				payload.color += info.material->Kreflectivity * refPayload.color;

			}
		}


		return info.time;
	}
	return 0.0f;
}

/*--- Display Function ---*/
//The main display function.
//This allows you to draw pixels onto the display by using GL_POINTS.
//Drawn every time an update is required.
//Students: This is the main file you'll need to modify or replace.
//The idea with this example function is the following:
//1)Clear the screen so we can draw a new frame
//2)Cast a ray into the scene for each pixel on the screen and use the returned color to render the pixel
//3)Flush the pipeline so that the instructions we gave are performed.
void DemoDisplay()
{
	// Set up the materials used in the scene
	Material white = Material();
		white.ambient = glm::vec3(1, 1, 1);
		white.diffuse = glm::vec3(1, 1, 1);
		white.specular = glm::vec3(1, 1, 1);
		white.specularExponent = 10;
		white.Klocal = 0.9;
		white.Kreflectivity = 0.1;

	Material whiteAbsorb = Material();
		whiteAbsorb.ambient = glm::vec3(1, 1, 1);
		whiteAbsorb.diffuse = glm::vec3(1, 1, 1);
		whiteAbsorb.specular = glm::vec3(1, 1, 1);
		whiteAbsorb.specularExponent = 1;
		whiteAbsorb.Klocal = 1.0;
		whiteAbsorb.Kreflectivity = 0;

	Material shinyGreen = Material();
		shinyGreen.ambient = glm::vec3(0, 1, 0);
		shinyGreen.diffuse = glm::vec3(0, 1, 0);
		shinyGreen.specular = glm::vec3(1, 1, 1);
		shinyGreen.specularExponent = 50;
		shinyGreen.Klocal = 0.8;
		shinyGreen.Kreflectivity = 0.2;

	Material red = Material();
		red.ambient = glm::vec3(1, 0, 0);
		red.diffuse = glm::vec3(1, 0, 0);
		red.specular = glm::vec3(1, 1, 1);
		red.specularExponent = 10;
		red.Klocal = 0.9;
		red.Kreflectivity = 0.1;

	Material blue = Material();
		blue.ambient = glm::vec3(0, 0, 1);
		blue.diffuse = glm::vec3(0, 0, 1);
		blue.specular = glm::vec3(1, 1, 1);
		blue.specularExponent = 10;
		blue.Klocal = 0.9;
		blue.Kreflectivity = 0.1;

	Material yellow = Material();
		yellow.ambient = glm::vec3(1, 1, 0);
		yellow.diffuse = glm::vec3(1, 1, 0);
		yellow.specular = glm::vec3(1, 1, 1);
		yellow.specularExponent = 50;
		yellow.Klocal = 1;
		yellow.Kreflectivity = 0;

	Material mirror = Material();
		mirror.ambient = glm::vec3(1, 1, 1);
		mirror.diffuse = glm::vec3(1, 1, 1);
		mirror.specular = glm::vec3(1, 1, 1);
		mirror.specularExponent = 50;
		mirror.Klocal = 0;
		mirror.Kreflectivity = 1;

	Material greyMirror = Material();
		greyMirror.ambient = glm::vec3(0.5, 0.5, 0.5);
		greyMirror.diffuse = glm::vec3(0.5, 0.5, 0.5);
		greyMirror.specular = glm::vec3(1, 1, 1);
		greyMirror.specularExponent = 50;
		greyMirror.Klocal = 0.4;
		greyMirror.Kreflectivity = 0.6;

	Material purple = Material();
		purple.ambient = glm::vec3(1, 0, 1);
		purple.diffuse = glm::vec3(1, 0, 1);
		purple.specular = glm::vec3(1, 1, 1);
		purple.specularExponent = 10;
		purple.Klocal = 1;
		purple.Kreflectivity = 0;

	Material black = Material();
		black.ambient = glm::vec3(0, 0, 0);
		black.diffuse = glm::vec3(0, 0, 0);
		black.specular = glm::vec3(1, 1, 1);
		black.specularExponent = 10;
		black.Klocal = 1;
		black.Kreflectivity = 0;

	Material pink = Material();
		pink.ambient = glm::vec3(1, 0.7, 0.7);
		pink.diffuse = glm::vec3(1, 0.7, 0.7);
		pink.specular = glm::vec3(1, 1, 1);
		pink.specularExponent = 10;
		pink.Klocal = 1;
		pink.Kreflectivity = 0;

	//------------------------------------------------------------//
	//                   CONTROL PANEL                            //
	//------------------------------------------------------------//
	// The position of the point light source
	lightPos = glm::vec3(0, 50, 125);
	// Turn on to send shadow rays and generate basic shadows
	activateShadows = true;
	// Turn on to activate local phong illumination
	activatePhong = true;
	// Turn on to generate and compute reflection rays
	activateReflections = true;
	// The maximum number of bounces for reflection rays
	maxReflections = 5;

	// Select the scene you wish to view
	scene = 1;
	// 1 - The basic scene with a sphere, triangle and planes
	// 2 - A scene with two spheres to test reflection
	// 3 - A scene to test reflection behind the camera
	// 4 - A box of mirrors to test bouncing reflections
	// 5 - Test of the new AxisAlignedBox object
	//------------------------------------------------------------//

	// Create the objects for the given scene
	switch (scene) {
	// The standard scene with a shiny green sphere and a grey triangular mirror in a room
	// The room has a red left hand wall, blue right hand wall and white ceiling/floor
	// The backwall is a perfect mirror
	case 1 : {
		// Planes
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), mirror))); // Backwall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(80, 0, 0), glm::vec3(-1, 0, 0), red))); // RHS wall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(-80, 0, 0), glm::vec3(1, 0, 0), blue))); // LHS wall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, -60, 0), glm::vec3(0, 1, 0), white))); // floor
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, 60, 0), glm::vec3(0, -1, 0), whiteAbsorb))); // ceiling

		// Spheres
		objects.push_back(unique_ptr<Object>(new Sphere(30, glm::vec3(40, -30, 70), shinyGreen)));

		// Triangles
		objects.push_back(unique_ptr<Object>(new Triangle(glm::vec3(-30, -60, 100),  glm::vec3(-0, -60, 60), glm::vec3(-40, -30, 80), greyMirror)));
		break;
	}
	// A large sphere of perfect mirror reflects a smaller red sphere
	// Used to double check the reflections look correct
	case 2 : {
		// Planes
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), white))); // Backwall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(80, 0, 0), glm::vec3(-1, 0, 0), red))); // RHS wall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(-80, 0, 0), glm::vec3(1, 0, 0), blue))); // LHS wall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, -60, 0), glm::vec3(0, 1, 0), white))); // floor
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, 60, 0), glm::vec3(0, -1, 0), whiteAbsorb))); // ceiling

		// Spheres
		objects.push_back(unique_ptr<Object>(new Sphere(20, glm::vec3(0, -40, 150), red)));
		objects.push_back(unique_ptr<Object>(new Sphere(40, glm::vec3(0, -20, 70), mirror)));

		// Triangles
		//objects.push_back(unique_ptr<Object>(new Triangle(glm::vec3(-70,-60,80),  glm::vec3(-40,-60,120),glm::vec3(0,0,80), shinyGreen)));
		break;
	}
	// Create a 'face' for the viewer behind the camera which can only be seen by the reflection in the backwall and small
	// triangular mirror
	case 3 : {
		// Planes
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), mirror))); // Backwall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(80, 0, 0), glm::vec3(-1, 0, 0), red))); // RHS wall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(-80, 0, 0), glm::vec3(1, 0, 0), blue))); // LHS wall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, -60, 0), glm::vec3(0, 1, 0), white))); // floor
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, 60, 0), glm::vec3(0, -1, 0), whiteAbsorb))); // ceiling

		// Spheres
		objects.push_back(unique_ptr<Object>(new Sphere(30, glm::vec3(0, 0, 250), pink)));
		objects.push_back(unique_ptr<Object>(new Sphere(5, glm::vec3(0, 0, 220), pink)));
		objects.push_back(unique_ptr<Object>(new Sphere(10, glm::vec3(-10, 10, 230), whiteAbsorb)));
		objects.push_back(unique_ptr<Object>(new Sphere(10, glm::vec3(10, 10, 230), whiteAbsorb)));
		objects.push_back(unique_ptr<Object>(new Sphere(5, glm::vec3(-10, 10, 222), black)));
		objects.push_back(unique_ptr<Object>(new Sphere(5, glm::vec3(10, 10, 222), black)));

		// Triangles
		objects.push_back(unique_ptr<Object>(new Triangle(glm::vec3(-30, -60, 140),  glm::vec3(-0, -60, 130), glm::vec3(-40, -30, 120), greyMirror)));
		//objects.push_back(unique_ptr<Object>(new Triangle(glm::vec3(-70,-60,80),  glm::vec3(-40,-60,120),glm::vec3(0,0,80), shinyGreen)));
		break;
	}
	// Shining a light into a mirrored box containing the basic pink 'face'
	case 4 : {
		lightPos = glm::vec3(0, 0, 200);
		// Planes
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), greyMirror))); // Backwall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(40, 0, 0), glm::vec3(-1, 0, 0), greyMirror))); // RHS wall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(-40, 0, 0), glm::vec3(1, 0, 0), greyMirror))); // LHS wall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, -30, 0), glm::vec3(0, 1, 0), greyMirror))); // floor
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, 30, 0), glm::vec3(0, -1, 0), greyMirror))); // ceiling

		// Spheres
		objects.push_back(unique_ptr<Object>(new Sphere(20, glm::vec3(0, -10, 100), pink)));
		objects.push_back(unique_ptr<Object>(new Sphere(5, glm::vec3(0, -10, 120), pink)));
		objects.push_back(unique_ptr<Object>(new Sphere(10, glm::vec3(-10, 0, 110), whiteAbsorb)));
		objects.push_back(unique_ptr<Object>(new Sphere(10, glm::vec3(10, 0, 110), whiteAbsorb)));
		objects.push_back(unique_ptr<Object>(new Sphere(5, glm::vec3(-10, 0, 118), black)));
		objects.push_back(unique_ptr<Object>(new Sphere(5, glm::vec3(10, 0, 118), black)));

		// Triangles
		//objects.push_back(unique_ptr<Object>(new Triangle(glm::vec3(-30,-60,140),  glm::vec3(-0,-60,130),glm::vec3(-40,-30,120), greyMirror)));
		//objects.push_back(unique_ptr<Object>(new Triangle(glm::vec3(-70,-60,80),  glm::vec3(-40,-60,120),glm::vec3(0,0,80), shinyGreen)));
		break;
	}
	// Test of additional feature - axis-aligned box
	case 5 : {
		// Planes
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), mirror))); // Backwall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(80, 0, 0), glm::vec3(-1, 0, 0), red))); // RHS wall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(-80, 0, 0), glm::vec3(1, 0, 0), blue))); // LHS wall
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, -60, 0), glm::vec3(0, 1, 0), white))); // floor
		objects.push_back(unique_ptr<Object>(new Plane(glm::vec3(0, 60, 0), glm::vec3(0, -1, 0), whiteAbsorb))); // ceiling

		// Spheres
		//objects.push_back(unique_ptr<Object>(new Sphere(30, glm::vec3(40,-30, 70), shinyGreen)));

		// Triangles
		//objects.push_back(unique_ptr<Object>(new Triangle(glm::vec3(-30,-60,100),  glm::vec3(-0,-60,60),glm::vec3(-40,-30,80), greyMirror)));

		//Boxes
		objects.push_back(unique_ptr<Object>(new AxisAlignedBox(glm::vec3(-50, -50, 100),  glm::vec3(-20, -20, 70), shinyGreen)));
		break;
	}
	default :
		break;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear OpenGL Window

	//The window aspect ratio
	float aspectRatio = (float)windowX / (float)windowY;
	//The field of view of the camera.  This is 90 degrees because our imaginary image plane is 2 units high (-1->1) and 1 unit from the camera position
	float fov = 90.0f;
	//Value for adjusting the pixel position to account for the field of view
	//float fovAdjust = tan(fov*0.5f *(M_PI/180.0f));
	float fovAdjust = tan(fov * 0.5f * (3.14f / 180.0f));

	//Tell OpenGL to start rendering points
	glBegin(GL_POINTS);

	//Set up our camera transformation matrices
	viewMatrix = glm::translate(glm::mat4(1.0f), originP);

	//Iterate over each pixel in the image
	for (int column = 0; column < windowX; ++column) {

		// if (column % (windowX / 20) == 0){
		// 	float perc = (((float)column / (float)windowX)*100);
		// 	cout << perc << "%" << endl;
		// }

		for (int row = 0; row < windowY; ++row) {

			//Convert the pixel (Raster space coordinates: (0->ScreenWidth,0->ScreenHeight)) to NDC (Normalised Device Coordinates: (0->1,0->1))
			float pixelNormX = (column + 0.5f) / windowX; //Add 0.5f to get centre of pixel
			float pixelNormY = (row + 0.5f) / windowY;
			//Convert from NDC, (0->1,0->1), to Screen space (-1->1,-1->1).  These coordinates correspond to those used by OpenGL
			//Note coordinate (-1,1) in screen space corresponds to coordinate (0,0) in raster space i.e. column = 0, row = 0
			float pixelScreenX = 2.0f * pixelNormX - 1.0f;
			float pixelScreenY = 1.0f - 2.0f * pixelNormY;

			//Account for Field of View
			float pixelCameraX = pixelScreenX * fovAdjust;
			float pixelCameraY = pixelScreenY * fovAdjust;

			//Account for image aspect ratio
			pixelCameraX *= aspectRatio;

			//Put pixel into camera space (offset by 1 unit along camera facing direction i.e. negative z axis)
			//vec4 so we can multiply with view matrix later
			glm::vec4 pixelCameraSpace(pixelCameraX, pixelCameraY, -1.0f, 1.0f);

			glm::vec4 rayOrigin(0.0f, 0.0f, 0.0f, 1.0f); //ray comes from camera origin

			//Transform from camera space to world space
			pixelCameraSpace = viewMatrix * pixelCameraSpace;
			rayOrigin = viewMatrix * rayOrigin;
			//rayOrigin = glm::vec4(0.0f,0.0f,200.0f,1.0f);
			//cout << "rayOrigin " << glm::vec3(rayOrigin) << endl;
			//Set up ray in world space
			Ray ray(glm::vec3(rayOrigin), //The origin of the ray we are casting
			        glm::normalize(glm::vec3(pixelCameraSpace - rayOrigin))//The direction the ray is travelling in
			       );

			//Structure for storing the information we get from casting the ray
			Payload payload;

			//Default color is white
			glm::vec3 color(1.0f);

			//Cast our ray into the scene
			float time = CastRay(ray, payload);
			if (time > 0.0f) { // > 0.0f indicates an intersection

				if (payload.shadowed) {
					color = 1.0f * payload.color;
				}
				else {
					float red = payload.color.x;
					float blue = payload.color.y;
					float green = payload.color.z;
					color = glm::vec3(red, blue, green);
				}

			}
			//Get OpenGL to render the pixel with the color from the ray
			glColor3f(color.x, color.y, color.z);
			glVertex3f(pixelScreenX, pixelScreenY, 0.0f);
		}
	}
	glEnd();

	glFlush();// Output everything (write to the screen)
}


//This function is called when a (normal) key is pressed
//x and y give the mouse coordinates when a keyboard key is pressed
void DemoKeyboardHandler(unsigned char key, int x, int y)
{
	if (key == 'm')
	{
		cout << "Mouse location: " << x << " " << y << endl;
	}

	cout << "Key pressed: " << key << endl;

	glutPostRedisplay();

}

//Program entry point.
//argc is a count of the number of arguments (including the filename of the program).
//argv is a pointer to each c-style string.
int main(int argc, char **argv)
{

	atexit(cleanup);
	cout << "Computer Graphics Assignment 2 Demo Program" << endl;

	//initialise OpenGL
	glutInit(&argc, argv);
	//Define the window size with the size specifed at the top of this file
	glutInitWindowSize(windowX, windowY);

	//Create the window for drawing with the title "SimpleExample"
	glutCreateWindow("CG-CW2");

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	//Set the function demoDisplay (defined above) as the function that
	//is called when the window must display
	glutDisplayFunc(DemoDisplay);// Callback function
	//similarly for keyboard input
	glutKeyboardFunc(DemoKeyboardHandler);

	//Run the GLUT internal loop
	glutMainLoop();// Display everything and wait
}
