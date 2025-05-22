#define GLM_ENABLE_EXPERIMENTAL
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

// Animation method implementation
Animation::Sprite Animation::InitSprite(std::string path, glm::vec4 hitbox) {
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
    sprite.hitbox = hitbox;
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

    allActions = { punch, kick, Iceattack, block, Counter };
}

actions::action actions::checkInputs(platform::Input& input, Player& player) {
    actions::action nullaction;
    nullaction.name = "none";
    if (input.isButtonHeld(platform::Button::A) && input.isButtonHeld(platform::Button::D)) {
        player.currentState.ismoving = false;
    }
    else {
        if (input.isButtonHeld(platform::Button::A)) { nullaction.name = "move_left"; }
        else if (input.isButtonHeld(platform::Button::D)) { nullaction.name = "move_right"; }
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
        platform::log("Player is in animation and cannot change state.");
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

bool Player::IsAbleToStartAnim(Player player) {
    return !player.currentState.IsInAnimation;
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
    if (player.currentState.IsInAnimation) {
        player.animation.frameTimer += deltaTime;
        if (player.animation.frameTimer >= player.animation.frameDuration) {
            player.animation.CurrentFrame++;
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

void renderPlayer::updateFrame(Player& player, gl2d::Renderer2D& renderer) {
    renderer.renderRectangle(
        { player.attributes.position, 384, 384 },
        player.animation.sprite.texture,
        Colors_White,
        { 0, 0 },
        0,
        player.animation.sprite.atlas.get(player.animation.CurrentFrame, 0)
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
