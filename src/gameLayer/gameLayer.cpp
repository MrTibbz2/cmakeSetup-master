#define GLM_ENABLE_EXPERIMENTAL
#include "gameLayer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include "platformInput.h"
#include "imgui.h"
#include <iostream>
#include <string>
#include <sstream>
#include "imfilebrowser.h"
#include <gl2d/gl2d.h>
#include <platformTools.h>
#include <IconsForkAwesome.h>
#include <imguiTools.h>
#include <logs.h>
#include <fstream>
#include "player.cpp"
#include <windows.h>

struct GameData
{
	glm::vec2 rectPos = {100,100};
	
	
}gameData;


gl2d::Renderer2D renderer;
gl2d::Texture BGTexture;
glm::vec2 bgTextureSize;
actions PlayerActions;
Player player1;
Player player2;
renderPlayer PlayerRenderer;
 //wouldnt use for anything other than initsprite. weird but will do.
HANDLE heap = GetProcessHeap();


void ReduceMemoryUsage() {
	HANDLE hProcess = GetCurrentProcess();

	// Empty the working set (trim unused pages)
	if (SetProcessWorkingSetSize(hProcess, -1, -1)) {
		std::cout << "Successfully reduced memory usage.\n";
	}
	else {
		std::cerr << "Failed to reduce memory usage. Error: " << GetLastError() << "\n";
	}
}

bool initGame()
{
	loadsprites Spriteloader;
	Animation animation;
	//initializing stuff for the renderer
	gl2d::init();
	renderer.create();
	
	//loading the saved data. Loading an entire structure like this makes savind game data very easy.
	
	platform::readEntireFile(RESOURCES_PATH "gameData.data", &gameData, sizeof(GameData));
	
	platform::log("Init, resources path:");

	platform::log(RESOURCES_PATH);

	auto bgpath = std::string(RESOURCES_PATH) + "assets/bg/bg2.png";
	//loading the texture
	
	BGTexture.loadFromFile(bgpath.c_str());
	bgTextureSize = BGTexture.GetSize();
	
	//loading the player sprites
	Spriteloader.loadPlayerSprites(player1, animation);
	

	// just a test but:
	//platform::log("Player1 sprite lenth for double_jump: " + player1.sprites_blue_right["Double_Jump"].length);
	player1.currentState.facing = "right"; // or "left" as appropriate
	player1.attributes.name = "blue";
	player1.setSprite(player1, "Defensive_Stance");
	player1.attributes.position = { 700, 350 };
	//platform::log(("Player1 sprite length for Ise_Strice: " + std::to_string(player1.sprites_blue_right["Ise_Strice"].length)).c_str());

	/*player1.sprites_blue_right.clear();
	player1.sprites_blue_left.clear();
	player1.sprites_red_left.clear();
	player1.sprites_red_right.clear();*/
	
	ReduceMemoryUsage();
	return true;
}


//IMPORTANT NOTICE, IF YOU WANT TO SHIP THE GAME TO ANOTHER PC READ THE README.MD IN THE GITHUB
//https://github.com/meemknight/cmakeSetup
//OR THE INSTRUCTION IN THE CMAKE FILE.
//YOU HAVE TO CHANGE A FLAG IN THE CMAKE SO THAT RESOURCES_PATH POINTS TO RELATIVE PATHS
//BECAUSE OF SOME CMAKE PROGBLMS, RESOURCES_PATH IS SET TO BE ABSOLUTE DURING PRODUCTION FOR MAKING IT EASIER.

bool gameLogic(float deltaTime, platform::Input& input)
{
#pragma region init stuff
	int w = 0; int h = 0;
	w = platform::getFrameBufferSizeX(); //window w
	h = platform::getFrameBufferSizeY(); //window h

	glViewport(0, 0, w, h);
	glClear(GL_COLOR_BUFFER_BIT); //clear screen

	renderer.updateWindowMetrics(w, h);
#pragma endregion

	//you can also do platform::isButtonHeld(platform::Button::Left)
	//
	//if (input.isButtonHeld(platform::Button::Left))
	//{
		//gameData.rectPos.x -= deltaTime * 100;
	//}
	//if (input.isButtonHeld(platform::Button::Right))
	//{
	//	gameData.rectPos.x += deltaTime * 100;
	//}
	//if (input.isButtonHeld(platform::Button::Up))
	//{
	//	gameData.rectPos.y -= deltaTime * 100;
	//}
	//if (input.isButtonHeld(platform::Button::Down))
	//{
	//	gameData.rectPos.y += deltaTime * 100;
	//}
	//gameData.rectPos = glm::clamp(gameData.rectPos, glm::vec2{0,0}, glm::vec2{w - 100,h - 100});


#pragma region background stuff

	renderer.renderRectangle({ 0, 0, bgTextureSize }, BGTexture);

#pragma endregion




	//renderer.renderRectangle({gameData.rectPos, 100, 100}, Colors_Blue);

	

	
	if (input.isButtonHeld(platform::Button::A) && input.isButtonHeld(platform::Button::D))
	{
		player1.currentState.ismoving = false;
	}
	else {
		if (input.isButtonHeld(platform::Button::A)) { player1.move(player1, "left", deltaTime, true); }
		else if (input.isButtonHeld(platform::Button::D)) { player1.move(player1, "right", deltaTime, false); }
		else { player1.currentState.ismoving = false; }
	}
	//if (player1.animation.sprite.texture.id == 0) {
	//	platform::log(("Warning: Invalid texture for sprite: " + player1.currentState.statename).c_str());
	//	// Optionally skip rendering this sprite
	//}
	PlayerRenderer.updatePlayer(player1, deltaTime, renderer);

	//renderer.renderRectangle({ player1.attributes.position, 128, 128 }, player1.animation.sprite.texture);

	renderer.flush();

	/*renderer.cleanup();*/





	return true;
};

//This function might not be be called if the program is forced closed
void closeGame()
{

	//saved the data.
	platform::writeEntireFile(RESOURCES_PATH "gameData.data", &gameData, sizeof(GameData));

}
