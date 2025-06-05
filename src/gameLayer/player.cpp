#define GLM_ENABLE_EXPERIMENTAL
#define NOMINMAX
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
#include "Player.h"
#include <algorithm>

// Animation method implementation
Animation::Sprite Animation::InitSprite(std::string path) {
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

    int frameWidth = 384;
    int frameCount = static_cast<int>(size.x / frameWidth);
    sprite.length = frameCount - 1;
    sprite.atlas = gl2d::TextureAtlas(frameCount, 1);
    return sprite;
}

// actions constructor
actions::actions() {
    punch.name = "punch";
    punch.type = "attack";
    punch.cancelable = false;
    punch.anim = "Punch_1";
    punch.damage = 10;
    punch.keybind = platform::Button::LeftShift;
    punch.staminaCost = 15;
    punch.Frames = 5;
    punch.hitframe = 3;

    kick.name = "kick";
    kick.type = "attack";
    kick.cancelable = false;
    kick.anim = "Kick";
    kick.damage = 20;
    kick.keybind = platform::Button::LeftCtrl;
    kick.staminaCost = 25;
    kick.Frames = 5;

    Iceattack.name = "Iceattack";
    Iceattack.type = "magic_attack";
    Iceattack.cancelable = false;
    Iceattack.anim = "Ise_Strice";
    Iceattack.damage = 10;
    Iceattack.keybind = platform::Button::W;
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

    isHit.name = "isHit";
	isHit.type = "effect";
	isHit.cancelable = false;
	isHit.anim = "None";
	isHit.damage = 0;
	isHit.keybind = 0;
	isHit.staminaCost = 0;
	isHit.Frames = 0;
	isHit.ApliesModifier = "none";
	isHit.HasModifier = "stun";
	isHit.ApliesModifierValue = 0;
	isHit.HasmodifierValue = 4;


    
    allActions = { punch, kick, Iceattack, block, Counter };
}

actions::action actions::checkInputs(platform::Input& input, Player& player, int leftkeybind, int rightkeybind) {

    actions::action nullaction;
    nullaction.name = "none";
    if (player.currentState.stunnedforframes > 0) { return nullaction; } // super easy stun logic 

    if (input.isButtonHeld(leftkeybind) && input.isButtonHeld(rightkeybind)) {
        player.currentState.ismoving = false;
    }
    else {
        if (input.isButtonHeld(leftkeybind)) { nullaction.name = "move_left"; }
        else if (input.isButtonHeld(rightkeybind)) { nullaction.name = "move_right"; }
        else { player.currentState.ismoving = false; }
    }
    for (const auto& action : allActions) {
        if (input.isButtonPressed(action.keybind)) {
            return action;
        }
    }
    return nullaction;
}

void actions::updateState(Player& player, actions::action action, float deltatime) {
    if (player.currentState.IsInAnimation && !player.currentState.CancellableAnimation) {
        
        return;
    }
    if (action.name == "none") {
        return;
    }
    if (action.name == "move_left") {
        player.move(player, "left", deltatime, true);
        return;
    }
    if (action.name == "move_right") {
        player.move(player, "right", deltatime, false);
        return;
    }
    player.currentState.currentAction = action;
    player.currentState.IsInAnimation = true;
    player.currentState.CancellableAnimation = action.cancelable;
    player.currentState.modifier = action.HasModifier;
    player.currentState.ismoving = false;
    player.setSprite(player, action.anim);
    player.animation.CurrentFrame = 0;
}


actions::action actions::checkHitboxes(Player& player1, Player& player2, effects& effectManager, effects::Effect hiteffect) {
    actions::action nullaction;
    nullaction.name = "none";

    if (player1.currentState.IsInAnimation) {
        if (player1.currentState.currentAction.name == "none" && player2.currentState.currentAction.name == "none") {
            //platform::log("Both players are not in an action, but yet are still in animation somehow (somethng is cooked)");
            return nullaction;
        }
        if (player1.currentState.Hitframe == true && !player2.currentState.IFrame && checkCollision(player1, player2)) {
            //platform::log("got hit");
            // Calculate intersection rectangle
            const glm::vec4& hb1 = player1.animation.sprite.hitbox; 
            const glm::vec4& hb2 = player2.animation.sprite.hitbox;
            glm::vec2 pos1 = player1.attributes.position + glm::vec2(hb1.x, hb1.y);
            glm::vec2 pos2 = player2.attributes.position + glm::vec2(hb2.x, hb2.y);

            float x1 = std::max(pos1.x, pos2.x);
            float y1 = std::max(pos1.y, pos2.y);
            float x2 = std::min(pos1.x + hb1.z, pos2.x + hb2.z);
            float y2 = std::min(pos1.y + hb1.w, pos2.y + hb2.w);

            glm::vec2 effectPos = { (x1 + x2) / 2.0f, (y1 + y2) / 2.0f };

            if (player2.currentState.currentAction.name == actions::block.name) {
                platform::log("Player 2 is blocking, no damage dealt");
                return actions::Counter;
            }
            else {
                //platform::log("Player 2 is not blocking, dealing damage");
                player2.attributes.health -= player1.currentState.currentAction.damage;
                player2.currentState.Iframes = 4;
				player2.currentState.IFrame = true;
				platform::log(("Player 2 health: " + std::to_string(player2.attributes.health)).c_str());
				platform::log(("Player 1 hit Player 2 with " + player1.currentState.currentAction.name + "for damage:" + std::to_string(player1.currentState.currentAction.damage)).c_str());
                player2.currentState.IsInAnimation = false;
                player2.currentState.ismoving = false;
                player2.currentState.currentAction = nullaction;
                effectManager.drawEffect(hiteffect, {effectPos.x, player1.attributes.position.y + 144.0f}, 0); // Draw hit effect at collision

                    
                
                
                return player1.currentState.currentAction;
            }
        }
    }
    return nullaction;
}
bool actions::checkCollision(const Player& player1, const Player& player2) {
    const glm::vec4& hb1 = player1.animation.sprite.hitbox;
    const glm::vec4& hb2 = player2.animation.sprite.hitbox;
    glm::vec2 pos1 = player1.attributes.position + glm::vec2(hb1.x, hb1.y);
    glm::vec2 pos2 = player2.attributes.position + glm::vec2(hb2.x, hb2.y);

    return (pos1.x < pos2.x + hb2.z && pos1.x + hb1.z > pos2.x &&
        pos1.y < pos2.y + hb2.w && pos1.y + hb1.w > pos2.y);
}

// Player methods
void Player::setSprite(Player& player, std::string spriteName) {
    Animation::Sprite* sprite = nullptr;
    if (spriteName.empty()) {
        platform::log("Error: setSprite called with empty spriteName!");
        return;
    }
    if (player.currentState.facing == "right" && player.attributes.name == "blue") {
        auto it = player.sprites_blue_right.find(spriteName);
        if (it != player.sprites_blue_right.end() && it->second.texture.id != 0) {
            sprite = &it->second;
        }
    }
    else if (player.currentState.facing == "left" && player.attributes.name == "blue") {
        auto it = player.sprites_blue_left.find(spriteName);
        if (it != player.sprites_blue_left.end() && it->second.texture.id != 0) {
            sprite = &it->second;
        }
    }
    else if (player.currentState.facing == "right" && player.attributes.name == "red") {
        auto it = player.sprites_red_right.find(spriteName);
        if (it != player.sprites_red_right.end() && it->second.texture.id != 0) {
            sprite = &it->second;
        }
    }
    else if (player.currentState.facing == "left" && player.attributes.name == "red") {
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
    }
}

void Player::move(Player& player, std::string direction, float deltatime, bool playbackwards) {
    if (player.currentState.IsInAnimation && !player.currentState.CancellableAnimation) {
        player.currentState.ismoving = false;
        return;
    }
    bool wasMoving = player.currentState.ismoving;
    bool wasBackwards = player.animation.playBackwards;

    player.setSprite(player, "Crawl");
    player.animation.playBackwards = playbackwards;
    player.currentState.ismoving = true;

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

void Player::regenManaAndStamina(Player& player, float deltatime) {
    player.attributes.mana += 10 * deltatime;
    player.attributes.stamina += 10 * deltatime;
    if (player.attributes.mana > player.attributes.maxMana) {
        player.attributes.mana = player.attributes.maxMana;
    }
    if (player.attributes.stamina > player.attributes.maxStamina) {
        player.attributes.stamina = player.attributes.maxStamina;
    }
}

// renderPlayer methods
int renderPlayer::updatePlayer(Player& player, float deltaTime, gl2d::Renderer2D& renderer) {
    if (player.currentState.IFrame) {
        player.currentState.IframeTimer += deltaTime;
        if (player.currentState.IframeTimer >= player.currentState.IframeLength) {
            player.currentState.Iframes--;
            player.currentState.IframeTimer -= player.currentState.IframeLength;
            if (player.currentState.Iframes <= 0) {
                player.currentState.IFrame = false;
                player.currentState.IframeTimer = 0.0f;
            }
        }
    }
    if (player.currentState.stunnedforframes > 0) {
        player.currentState.stunnedTimer += deltaTime;
        if (player.currentState.stunnedTimer >= player.currentState.stunnedLength) {
            player.currentState.stunnedforframes--;
            player.currentState.stunnedTimer -= player.currentState.stunnedLength;
            if (player.currentState.stunnedforframes <= 0) {
               
                player.currentState.stunnedTimer = 0.0f;
                player.currentState.stunnedforframes = 0;
            }
        }
    }
    if (player.currentState.IsInAnimation) {
        player.animation.frameTimer += deltaTime;
        if (player.animation.frameTimer >= player.animation.frameDuration) {

            player.animation.CurrentFrame++;

            if (player.animation.CurrentFrame == player.currentState.currentAction.hitframe) { player.currentState.Hitframe = true; }
            else { player.currentState.Hitframe = false; }

            player.animation.frameTimer -= player.animation.frameDuration;

            if (player.animation.CurrentFrame > player.animation.sprite.length) {
                platform::log("Animation finished, reseting to idle");
                player.animation.CurrentFrame = 0;
                player.setSprite(player, "Defensive_Stance");
                player.currentState.IsInAnimation = false;
                player.currentState.ismoving = false;
                return 1;
            }
        }
    }
    else if (player.currentState.ismoving) {
        player.animation.frameTimer += deltaTime;
        if (player.animation.frameTimer >= player.animation.frameDuration) {
            player.animation.frameTimer -= player.animation.frameDuration;
            if (player.animation.playBackwards) {
                player.animation.CurrentFrame--;
                if (player.animation.CurrentFrame < 0) {
                    player.animation.CurrentFrame = player.animation.sprite.length;
                }
            }
            else {
                player.animation.CurrentFrame++;
                if (player.animation.CurrentFrame > player.animation.sprite.length) {
                    player.animation.CurrentFrame = 0;
                }
            }
        }
    }
    else {
        player.animation.CurrentFrame = 0;
        player.setSprite(player, "Defensive_Stance");
        player.currentState.IsInAnimation = false;
    }
    updateFrame(player, renderer);
    return 0;
}


void renderPlayer::drawStatBars(Player player1, Player player2, gl2d::Renderer2D& renderer) {
    glm::vec2 player1barpos = { 50.0f, 75.0f };
    glm::vec2 player2barpos = {};

    int width = 200;
    int height = 50;
    player2barpos.x += 1920.0f - width - player1barpos.x;
    glm::vec4 P1basebar = { player1barpos.x, player1barpos.y, width, height };
    glm::vec4 P2basebar = { player2barpos.x, player1barpos.y , width, height };
    renderer.renderRectangle(P1basebar, Colors_Red);
    renderer.renderRectangle(P2basebar, Colors_Red);
    glm::vec4 P1healthbar = { player1barpos.x, player1barpos.y, float((player1.attributes.health / float(player1.attributes.maxHealth)) * width), height };
    glm::vec4 P2healthbar = { player2barpos.x, player1barpos.y, float((player2.attributes.health / float(player2.attributes.maxHealth)) * width), height };  
    renderer.renderRectangle(P1healthbar, Colors_Green);  // extra float is to force c++ to use float division  ^^^^
    renderer.renderRectangle(P2healthbar, Colors_Green);
}

void renderPlayer::updateFrame(Player& player, gl2d::Renderer2D& renderer) {
    renderer.renderRectangle(
        { (player.attributes.position.x + player.animation.sprite.drawOffset.x), (player.attributes.position.y + player.animation.sprite.drawOffset.y) , 384, 384},
        player.animation.sprite.texture,
        Colors_White,
        { 0, 0 },
        0,
        player.animation.sprite.atlas.get(player.animation.CurrentFrame, 0)
    );
}
void renderPlayer::showHitbox(Player& player, gl2d::Renderer2D& renderer) {
	renderer.renderRectangleOutline(
		{ player.attributes.position.x + player.animation.sprite.hitbox.x,
		  player.attributes.position.y + player.animation.sprite.hitbox.y,
		  player.animation.sprite.hitbox.z,
		  player.animation.sprite.hitbox.w },
        Colors_Red, 6.0f
	);


}
// loadsprites methods
void loadsprites::loadPlayerSprites(Player& player, Animation& animationLoader) {
    namespace fs = std::filesystem;
    const std::string baseDir = RESOURCES_PATH + std::string("assets/sprites/");

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

effects::Effect effects::loadEffect(std::string path, glm::vec2 atlasSize) {
	effects::Effect effect;
	effect.path = path;
	const std::string realpath = RESOURCES_PATH + std::string("assets/effects/") + path + std::string(".png");
    platform::log(("Trying to load sprite from: " + realpath).c_str());

    std::ifstream file(realpath);
    if (!file.good()) {
        platform::log(("ERROR: Sprite file does not exist or cannot be opened: " + path).c_str());
    }
    file.close();
    
    effect.texture.loadFromFile(realpath.c_str());
    
    effect.framesize.x = (effect.texture.GetSize().x / atlasSize.x);
    effect.framesize.y = (effect.texture.GetSize().y / atlasSize.y);
	effect.atlas = gl2d::TextureAtlas(atlasSize.x, atlasSize.y);\
	effect.length = (atlasSize.x - 1); // Assuming atlasSize is in frames
    return effect;
}

void effects::drawEffect(effects::Effect effect, glm::vec2 pos, int frameY) {
    effect.currentframe.x = 0;
    effect.pos = pos;
    effects::activeEffects.insert({ std::string(effect.path), effect });
}
void effects::updateEffects(float dt, gl2d::Renderer2D& renderer) {
	for (auto it = activeEffects.begin(); it != activeEffects.end();) {
		
		
        effects::Effect& effect = it->second;

		
        renderer.renderRectangle(
            { effect.pos.x, effect.pos.y, effect.framesize.x, effect.framesize.y },
            effect.texture,
            Colors_White,
            { 0, 0 },
            0,
            effect.atlas.get(effect.currentframe.x, effect.currentframe.y)
        );
        effect.frameTimer += dt;
        if (effect.frameTimer >= effect.frameDuration) {

            effect.frameTimer -= effect.frameDuration;
            effect.currentframe.x++;
        }
		
		if (effect.currentframe.x > effect.length) {
			it = activeEffects.erase(it); // Remove effect if it has finished
			continue;
		}
		
		++it;
	}
}

