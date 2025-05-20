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
#include <filesystem>
#include <unordered_map>

class Animation {
public:
	//animation class. used to track sprites and the currentanimation/textureAtlas (spritesheet)
	struct Sprite {
		std::string path;
		int length = 0; // Initialize length to 0
		gl2d::Texture texture;
		gl2d::TextureAtlas atlas;
		glm::vec4 hitbox = { 0.0f, 0.0f, 0.0f, 0.0f }; // Initialize hitbox to default values

		Sprite() = default; // Default constructor
	};

	Animation::Sprite Animation::InitSprite(std::string path, glm::vec4 hitbox = {0,0,0,0})
	{
		Animation::Sprite sprite;
		sprite.path = path;

		platform::log(("Trying to load sprite from: " + path).c_str());

		std::ifstream file(path);
		if (!file.good()) {
			platform::log(("ERROR: Sprite file does not exist or cannot be opened: " + path).c_str());
		}
		file.close();

		sprite.texture.loadFromFile(path.c_str());
		glm::vec2 size = sprite.texture.GetSize();

		// Each frame is 384x384, so number of frames is width / 384
		int frameWidth = 384;
		int frameCount = static_cast<int>(size.x / frameWidth);
		sprite.length = frameCount - 1; // 0-based indexing for frames
		sprite.atlas = gl2d::TextureAtlas(frameCount, 1);
		sprite.hitbox = hitbox;
		return sprite;
	}
};
class Player {
	// player class. used to track all about the player, including the texture/sprite sheet, nested.
public:
	std::unordered_map<std::string, Animation::Sprite> sprites_blue_right;
	std::unordered_map<std::string, Animation::Sprite> sprites_blue_left;
	std::unordered_map<std::string, Animation::Sprite> sprites_red_right;
	std::unordered_map<std::string, Animation::Sprite> sprites_red_left;
	
	struct state {
		bool IsInAnimation = false;
		bool CancellableAnimation = true;
		std::string statename;
		std::string modifier;
		std::string facing;
		bool IFrame = false;
		bool ismoving = false;
	};
	struct attributes {
		std::string name;
		glm::vec2 position = {0.0f, 0.0f};
		float x_offset = 0.0f;
		float y_offset = 0.0f;
		int health = 100;
		int maxHealth = 100;
		int mana = 100;
		int stamina = 100;
		int maxStamina = 100;
		int maxMana = 100;

	};
	struct animation {
		Animation::Sprite sprite;
		int CurrentFrame = 0;
		float frameTimer = 0.0f;
		float frameDuration = 0.1f;
		bool playBackwards = false;


	};
	
	state currentState;
	attributes attributes; // need to instantiate this struct for it to be a valid member of the class
	animation animation;
	
	void setSprite(Player& player, std::string spriteName) {
		Animation::Sprite* sprite = nullptr;
		if (player.currentState.facing == "right" && player.attributes.name == "blue") {
			//platform::log(("Setting sprite to blue_right" + spriteName).c_str());
			auto it = player.sprites_blue_right.find(spriteName);
			if (it != player.sprites_blue_right.end() && it->second.texture.id != 0) {
				sprite = &it->second;
			}
		}
		else if (player.currentState.facing == "left" && player.attributes.name == "blue") {
			//platform::log(("Setting sprite to blue_left" + spriteName).c_str());
			auto it = player.sprites_blue_left.find(spriteName);
			if (it != player.sprites_blue_left.end() && it->second.texture.id != 0) {
				sprite = &it->second;
			}
		}
		else if (player.currentState.facing == "right" && player.attributes.name == "red") {
			//platform::log(("Setting sprite to red_right" + spriteName).c_str());
			auto it = player.sprites_red_right.find(spriteName);
			if (it != player.sprites_red_right.end() && it->second.texture.id != 0) {
				sprite = &it->second;
			}
		}
		else if (player.currentState.facing == "left" && player.attributes.name == "red") {
			//platform::log(("Setting sprite to red_left" + spriteName).c_str());
			auto it = player.sprites_red_left.find(spriteName);
			if (it != player.sprites_red_left.end() && it->second.texture.id != 0) {
				sprite = &it->second;
			}
		}

		if (sprite) {
			player.animation.sprite = *sprite;
		}
		else {
			platform::log(("Error: Sprite '" + spriteName + "' not found or invalid texture!").c_str());
			// Optionally set a fallback sprite here
		}
	}

	bool IsAbleToStartAnim(Player player)
	{
		if (player.currentState.IsInAnimation == false)
		{
			return true;
		}
		else
		{
			return false;
		}


	};

	void move(Player& player, std::string direction, float deltatime, bool playbackwards) {
		if (player.currentState.IsInAnimation == true && player.currentState.CancellableAnimation == false) {
			player.currentState.ismoving = false;
			return;
		}

		bool wasMoving = player.currentState.ismoving;
		bool wasBackwards = player.animation.playBackwards;

		player.setSprite(player, "Crawl");
		player.animation.playBackwards = playbackwards;
		player.currentState.ismoving = true;

		// Only reset frame if starting to move or changing direction
		if (!wasMoving || wasBackwards != playbackwards) {
			if (playbackwards)
				player.animation.CurrentFrame = player.animation.sprite.length;
			else
				player.animation.CurrentFrame = 0;
		}

		if (direction == "left") {
			player.attributes.position.x -= 100 * deltatime;
		}
		else if (direction == "right") {
			player.attributes.position.x += 100 * deltatime;
		}
	}
	


	void regenManaAndStamina(Player& player, float deltatime) {
		// regen
		player.attributes.mana += 10 * deltatime;
		player.attributes.stamina += 10 * deltatime;
		// clamp to max
		if (player.attributes.mana > player.attributes.maxMana) {
			player.attributes.mana = player.attributes.maxMana;
		}
		if (player.attributes.stamina > player.attributes.maxStamina) {
			player.attributes.stamina = player.attributes.maxStamina;
		}
	}
};

class renderPlayer {
	//used to update player's sprite/render
public:
	int updatePlayer(Player& player, float deltaTime, gl2d::Renderer2D& renderer)
	{
		if (player.currentState.IsInAnimation)
		{
			player.animation.frameTimer += deltaTime;
			if (player.animation.frameTimer >= player.animation.frameDuration) {
				player.animation.CurrentFrame++;
				player.animation.frameTimer -= player.animation.frameDuration;

				if (player.animation.CurrentFrame > player.animation.sprite.length) {
					platform::log("Animation finished, reseting to idle");
					player.animation.CurrentFrame = 0; // Reset frame here
					player.setSprite(player, "Defensive_Stance");
					player.currentState.IsInAnimation = false;
					player.currentState.modifier = "none";
					return 1;
				}
			}
		}
		else if (player.currentState.ismoving == true)
		{
			player.animation.frameTimer += deltaTime;
			if (player.animation.frameTimer >= player.animation.frameDuration) {
				player.animation.frameTimer -= player.animation.frameDuration;
				if (player.animation.playBackwards == true) {
					// Play Crawl backwards
					player.animation.CurrentFrame--;
					if (player.animation.CurrentFrame < 0) {
						player.animation.CurrentFrame = player.animation.sprite.length;
					}
				}
				else {
					// Play forward
					player.animation.CurrentFrame++;
					if (player.animation.CurrentFrame > player.animation.sprite.length) {
						player.animation.CurrentFrame = 0;
					}
				}
			}
		}
		else {
			player.animation.CurrentFrame = 0; // Reset frame here
			player.setSprite(player, "Defensive_Stance");
			player.currentState.IsInAnimation = false;
		}
		// Always render the current frame
		updateFrame(player, renderer);
		return 0;
	}
	void updateFrame(Player& player, gl2d::Renderer2D& renderer) {
		// used only for rendering.
		renderer.renderRectangle(
			{ player.attributes.position, 384, 384 },
			player.animation.sprite.texture,
			Colors_White,
			{ 0, 0 },
			0,
			player.animation.sprite.atlas.get(player.animation.CurrentFrame, 0)
		);
		
	}
};
class loadsprites
{
	//used to load the sprites from the file system
public:
	void loadPlayerSprites(Player& player, Animation& animationLoader) {
		namespace fs = std::filesystem;
		const std::string baseDir = RESOURCES_PATH + std::string("assets/sprites/");

		// Map directory names to the corresponding Player member
		std::vector<std::pair<std::string, std::unordered_map<std::string, Animation::Sprite>*>> spriteDirs = {
			{"blue_right", &player.sprites_blue_right},
			{"blue_left",  &player.sprites_blue_left},
			{"red_right",  &player.sprites_red_right},
			{"red_left",   &player.sprites_red_left}
		};

		for (auto& [dirName, spriteMap] : spriteDirs) {
			spriteMap->clear();
			fs::path dirPath = baseDir + dirName;
			if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) continue;

			for (const auto& entry : fs::directory_iterator(dirPath)) {
				if (entry.is_regular_file() && entry.path().extension() == ".png") {
					std::string spriteName = entry.path().stem().string();
					platform::log(("Loading sprite: " + spriteName).c_str());
					std::string fixedPath = entry.path().string();
					std::replace(fixedPath.begin(), fixedPath.end(), '\\', '/');
					(*spriteMap)[spriteName] = animationLoader.InitSprite(fixedPath);
				}
			}
		}
	}
};

class actions
	//used to track the actions of the player, including attacks, blocks, etc.
{
public:	//used to complete moves and actions for the player, set states, anims, etc
	struct action {
		std::string name = "";
		std::string type = "";
		bool cancelable = false;
		std::string ApliesModifier = "none";
		std::string HasModifier = "none";
		int HasmodifierValue = 0;
		int ApliesModifierValue = 0;
		std::string anim = "";
		glm::vec4 hitbox = {0,0,0,0};
		int damage = 0;
		int keybind = 0;
		int Frames = 0;
		int manaCost = 0;
		int staminaCost = 0;

	};
	action punch;
	action kick;
	action Iceattack;
	action block;
	action Counter;
	

#pragma region action functions;
	actions() {
		punch.name = "punch";
		punch.type = "attack";
		punch.cancelable = false;
		punch.anim = "Punch_1";
		punch.damage = 10;
		punch.keybind = platform::Button::LeftShift;
		punch.staminaCost = 15;
		punch.Frames = 5;



		kick.name = "kick";
		kick.type = "attack";
		kick.cancelable = false;
		kick.anim = "Kick";
		kick.damage = 20;
		kick.keybind = platform::Button::LeftCtrl;
		kick.staminaCost = 25;
		kick.Frames = 5;
		
		Iceattack.name = "Iceattack";
		Iceattack.type = "attack";
		Iceattack.cancelable = false;
		Iceattack.anim = "Ise_Strice";
		Iceattack.damage = 10;
		Iceattack.keybind = platform::Button::Space;
		Iceattack.staminaCost = 15;
		Iceattack.manaCost = 75;
		Iceattack.Frames = 10;
		Iceattack.ApliesModifier = "slow";
		Iceattack.ApliesModifierValue = 2;


		block.name = "block";
		block.type = "defense";
		block.cancelable = true;
		block.anim = "Protect";
		block.damage = 0;
		block.keybind = platform::Button::LeftAlt;
		block.staminaCost = 5;
		block.Frames = 2;
		block.HasModifier = "defense";
		block.HasmodifierValue = 50;



		Counter.name = "Counter";
		Counter.type = "attack";
		Counter.cancelable = false;
		Counter.anim = "Defense attack";
		Counter.damage = 30;
		Counter.keybind = platform::Button::LeftAlt;
		Counter.staminaCost = 10;
		Counter.Frames = 4;
		Counter.ApliesModifier = "stun";
		Counter.ApliesModifierValue = 5;




		
	}
	std::vector<actions::action> allActions = { punch, kick, Iceattack, block, Counter };
#pragma endregion

	void perfAction(Player& player) {
		if (player.currentState.IsInAnimation == true && player.currentState.CancellableAnimation == false)
		{
			platform::log("Player is in animation, cannot perform action");
			return;
		}
		else {
			for (actions::action& act : allActions) {
				if (platform::isButtonPressed(act.keybind)) { // or your input system

					player.setSprite(player, act.anim);
					player.currentState.IsInAnimation = true;
					player.currentState.CancellableAnimation = act.cancelable;
					player.animation.CurrentFrame = 0;
					player.currentState.statename = act.name;
					player.currentState.modifier = act.HasModifier;
					platform::log(("Performing action: " + act.name).c_str());



				}
			}
			
		}
	}
};