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
#include "player.h"
#include <windows.h>
#include <thread>
struct GameData
{
	glm::vec2 rectPos = {100,100};
	
	
}gameData;


gl2d::Renderer2D renderer;
gl2d::Texture BGTexture;
glm::vec2 bgTextureSize;
actions Player1Actions;
actions Player2Actions;
Player player1;
Player player2;
renderPlayer PlayerRenderer;
 //wouldnt use for anything other than initsprite. weird but will do.
HANDLE heap = GetProcessHeap();
effects EffectsManager;
effects::Effect hiteffect;
void initializeEffects() {
	hiteffect = EffectsManager.loadEffect("Hit_8x9", { 8, 9 });
	hiteffect.frameDuration = 0.03f; // Set frame duration for the effect
}

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
void ScheduleMemoryTrimmingEvery(DWORD milliseconds) {
	std::thread([milliseconds]() {
		while (true) {
			ReduceMemoryUsage();
			std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
		}
		}).detach();
}

bool initGame()
{
	loadsprites Spriteloader;
	Animation animation1;

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
	Spriteloader.loadPlayerSprites(player1, animation1);
	player2.sprites_blue_left = player1.sprites_blue_left; 
	player2.sprites_blue_right = player1.sprites_blue_right; 
	player2.sprites_red_left = player1.sprites_red_left; 
	player2.sprites_red_right = player1.sprites_red_right; // this way of loading it is WAY faster. in a perfect world the srpites wouldnt be stored under a player class but rather a global class, but this is fine for a simple game like this.
	

	// just a test but:
	//platform::log("Player1 sprite lenth for double_jump: " + player1.sprites_blue_right["Double_Jump"].length);
	player1.currentState.facing = "right"; // or "left" as appropriate
	player1.attributes.name = "blue";
	player1.setSprite(player1, "Defensive_Stance");
	player2.setSprite(player2, "Defensive_Stance");
	player1.attributes.position = { 700, 336 };
	player2.attributes.position = { 1100, 336 };
	player2.currentState.facing = "left"; // or "right" as appropriate
	player2.attributes.name = "red";

	//platform::log(("Player1 sprite length for Ise_Strice: " + std::to_string(player1.sprites_blue_right["Ise_Strice"].length)).c_str());

	/*player1.sprites_blue_right.clear();
	player1.sprites_blue_left.clear();
	player1.sprites_red_left.clear();
	player1.sprites_red_right.clear();*/
	


	


	player1.sprites_blue_right["Ise_Strice"].drawOffset = { 92.0f, 0.0f};
	player1.sprites_blue_left["Ise_Strice"].drawOffset = { 92.0f, 0.0f };
	player1.sprites_red_right["Ise_Strice"].drawOffset = { 92.0f, 0.0f };
	player1.sprites_red_left["Ise_Strice"].drawOffset = { 92.0f, 0.0f };
	player2.sprites_blue_right["Ise_Strice"].drawOffset = { -92.0f, 0.0f };
	player2.sprites_blue_left["Ise_Strice"].drawOffset = { -92.0f, 0.0f };
	player2.sprites_red_right["Ise_Strice"].drawOffset = { -92.0f, 0.0f };
	player2.sprites_red_left["Ise_Strice"].drawOffset = { -92.0f, 0.0f };

	Player1Actions.allActions.clear();
	Player2Actions.allActions.clear();
	Player2Actions.punch.keybind = platform::Button::P;
	Player2Actions.kick.keybind = platform::Button::K;
	Player2Actions.Iceattack.keybind = platform::Button::L;
	Player2Actions.block.keybind = platform::Button::Enter;
	Player2Actions.Counter.keybind = -1;

	Player1Actions.punch.keybind = platform::Button::LeftShift;
	Player1Actions.kick.keybind = platform::Button::LeftCtrl;
	Player1Actions.Iceattack.keybind = platform::Button::W;
	Player1Actions.block.keybind = platform::Button::LeftAlt;
	Player1Actions.Counter.keybind = -1;

	Player1Actions.allActions.push_back(Player1Actions.punch);
	Player1Actions.allActions.push_back(Player1Actions.kick);
	Player1Actions.allActions.push_back(Player1Actions.Iceattack);
	Player1Actions.allActions.push_back(Player1Actions.block);  // 
	Player1Actions.allActions.push_back(Player1Actions.Counter);

	Player2Actions.allActions.push_back(Player2Actions.punch);
	Player2Actions.allActions.push_back(Player2Actions.kick);
	Player2Actions.allActions.push_back(Player2Actions.Iceattack);
	Player2Actions.allActions.push_back(Player2Actions.block);  //
	Player2Actions.allActions.push_back(Player2Actions.Counter);
	initializeEffects();
	ScheduleMemoryTrimmingEvery(10000); // Schedule memory trimming every 10 seconds
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




	
	

	

	//if (player1.animation.sprite.texture.id == 0) {
	//	platform::log(("Warning: Invalid texture for sprite: " + player1.currentState.statename).c_str());
	//	// Optionally skip rendering this sprite
	//}
	
	/*if (input.isButtonPressed(platform::Button::Escape))
	{
		EffectsManager.drawEffect(hiteffect, { 100.0f, 100.0f }, 0);
	}*/
	actions::action action = Player1Actions.checkInputs(input, player1, platform::Button::A, platform::Button::D);
	actions::action action2 = Player2Actions.checkInputs(input, player2, platform::Button::Left, platform::Button::Right);
	actions::action result1, result2 = Player1Actions.checkHitboxes(player1, player2, EffectsManager, hiteffect); Player2Actions.checkHitboxes(player2, player1, EffectsManager, hiteffect);
	/*platform::log(("Player1 action: " + result1.name).c_str());
	platform::log(("Player2 action: " + result2.name).c_str());*/

	
	Player1Actions.updateState(player1, action, deltaTime);

	
	
	Player2Actions.updateState(player2, action2, deltaTime); 
	
	
	PlayerRenderer.updatePlayer(player1, deltaTime, renderer);
	PlayerRenderer.updatePlayer(player2, deltaTime, renderer);
	PlayerRenderer.showHitbox(player2, renderer);
	PlayerRenderer.showHitbox(player1, renderer);
	PlayerRenderer.drawStatBars(player1, player2, renderer);
	EffectsManager.updateEffects(deltaTime, renderer);
	renderer.flush();




	

	return true;
};

//This function might not be be called if the program is forced closed
void closeGame()
{

	//saved the data.
	platform::writeEntireFile(RESOURCES_PATH "gameData.data", &gameData, sizeof(GameData));

}
