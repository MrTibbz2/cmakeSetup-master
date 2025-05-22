#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <gl2d/gl2d.h>
#include <platformInput.h>

// Forward declarations
class actions;
class Player;
class Animation;

// Animation class
class Animation {
public:
    struct Sprite {
        std::string path;
        int length = 0;
        gl2d::Texture texture;
        gl2d::TextureAtlas atlas;
        glm::vec4 hitbox = { 0.0f, 0.0f, 0.0f, 0.0f };

        Sprite() = default;
    };

    Sprite InitSprite(std::string path, glm::vec4 hitbox = { 0,0,0,0 });
};

// actions class
class actions {
public:
    struct action {
        std::string name = "";
        std::string type = "";
        bool cancelable = false;
        std::string ApliesModifier = "none";
        std::string HasModifier = "none";
        int HasmodifierValue = 0;
        int ApliesModifierValue = 0;
        std::string anim = "";
        glm::vec4 hitbox = { 0,0,0,0 };
        int damage = 0;
        int keybind = 0;
        int Frames = 0;
        int manaCost = 0;
        int staminaCost = 0;
        int range = 0;
    };

    action punch;
    action kick;
    action Iceattack;
    action block;
    action Counter;

    std::vector<action> allActions;

    actions();

    action checkInputs(platform::Input& input, Player& player);
    void updateState(Player& player, action action, float deltatime);
};

// Player class
class Player {
public:
    std::unordered_map<std::string, Animation::Sprite> sprites_blue_right;
    std::unordered_map<std::string, Animation::Sprite> sprites_blue_left;
    std::unordered_map<std::string, Animation::Sprite> sprites_red_right;
    std::unordered_map<std::string, Animation::Sprite> sprites_red_left;

    struct state {
        bool IsInAnimation = false;
        bool CancellableAnimation = true;
        std::string statename;
        actions::action currentAction;
        std::string modifier;
        std::string facing;
        bool IFrame = false;
        bool ismoving = false;
    };
    struct attributes {
        std::string name;
        glm::vec2 position = { 0.0f, 0.0f };
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
        float x_offset = 0.0f;
        float y_offset = 0.0f;
    };

    state currentState;
    attributes attributes;
    animation animation;

    void setSprite(Player& player, std::string spriteName);
    bool IsAbleToStartAnim(Player player);
    void move(Player& player, std::string direction, float deltatime, bool playbackwards);
    void regenManaAndStamina(Player& player, float deltatime);
};

// renderPlayer class
class renderPlayer {
public:
    int updatePlayer(Player& player, float deltaTime, gl2d::Renderer2D& renderer);
    void updateFrame(Player& player, gl2d::Renderer2D& renderer);
};

// loadsprites class
class loadsprites {
public:
    void loadPlayerSprites(Player& player, Animation& animationLoader);
};
