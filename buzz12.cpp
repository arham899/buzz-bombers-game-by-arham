#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <fstream>
#define MAX_POWER_UPS 100
#define MAX_INFANT_BEES 50
using namespace sf;
using namespace std;



int nextInfantBeeIndex =0;
// Game constants
const int resolutionX = 960;
const int resolutionY = 640;
const int rowHeight = resolutionY / 8;
const int boxPixelsX = 32;
const int boxPixelsY = 40;
int maxBees = 20;
int maxHoneycombs = 30;
int maxFlowers = 31;
int lives = 3, score = 0, sprayCount = 56;
Font font;
const int MAX_HIGH_SCORES = 5;  // Maximum number of high scores
std::string playerNames[MAX_HIGH_SCORES];
int playerScores[MAX_HIGH_SCORES];

// Structures

struct Direction {
    float x;
    float y;

    Direction(float x = 0.f, float y = 0.f) : x(x), y(y) {}
};

struct Bee {
    Sprite sprite;
    bool isAlive = true;
    bool isKiller = false;
    bool isExiting = false;
    float speed = 2.0f;
    int beeDirection = 1;
    bool isStopped = false;  // Whether the bee is currently stopped
    Clock stopTimer;
};

struct PowerUp {
    Sprite sprite;
    int type; // 1: Speed Increase, 2: Speed Decrease, 3: Height Increase, 4: Height Decrease
    bool isActive;
    Clock timer;
    float duration; // Duration in seconds
};

struct InfantBee {
    Sprite sprite;
    bool isAlive;
    bool isTransformed;
    Direction direction;
    Clock spawnTimer;
    Clock movementTimer;
};
struct Hummingbird {
    Sprite sprite;
    bool isAlive = false;
    int sicknessCount = 0;      // Tracks how many times the bird has been hit
    Clock moveTimer;           // Timer for pause and movement
    Clock respawnTimer;        // Timer for reappearing after sickness exit
    Clock waitTimer;           // Timer for waiting after eating a honeycomb
    Direction direction;       // Current direction of movement
    Clock birdAppearanceTimer; // Timer for tracking hummingbird appearance
    bool isWaiting = false;    // Indicates if the hummingbird is deciding its direction
    bool isPausedAfterHoneycomb = false; // Indicates if the bird is waiting after honeycomb collision
};

struct Honeycomb {
    Sprite sprite;
    bool collection_start = false;
    Clock honeycomb_collection;
    bool isCollected = false;
};

struct Flower {
    Sprite sprite;
};
int nextPowerUpIndex = 0; 
PowerUp powerUps[MAX_POWER_UPS];
InfantBee infantBees[MAX_INFANT_BEES];
void initializePowerUp(PowerUp& powerUp, Texture& texture, int type, float x, float y);
void initializeInfantBee(InfantBee& infantBee, Texture& texture, float x, float y);
// High Score File
const std::string highScoreFile = "highscores.txt";

// Function to load the high scores from file
void loadHighScores() {
    std::ifstream file(highScoreFile);
    if (file.is_open()) {
        int i = 0;
        std::string name;
        int score;
        while (file >> name >> score && i < MAX_HIGH_SCORES) {
            playerNames[i] = name;
            playerScores[i] = score;
            i++;
        }
        file.close();
    }
}

// Function to save the high scores to file
void saveHighScores() {
    std::ofstream file(highScoreFile, std::ios::trunc); // Open in trunc mode to overwrite the file
    if (file.is_open()) {
        for (int i = 0; i < MAX_HIGH_SCORES; i++) {
            if (playerScores[i] != 0) { // Only save non-zero scores
                file << playerNames[i] << " " << playerScores[i] << "\n";
            }
        }
        file.close();
    }
}

// Function to insert the new score and keep the list sorted
void insertHighScore(const std::string& playerName, int playerScore) {
    // Load the existing high scores first
    loadHighScores();

    // Find the correct position to insert the new score
    int position = -1;
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        // Insert the new score if it's higher than a score that's already 0 or if it's higher than any existing score
        if (playerScores[i] == 0 || playerScore > playerScores[i]) {
            position = i;
            break;
        }
    }

    // If the score fits in the list, shift others and insert it
    if (position != -1) {
        // Shift scores down to make space for the new score
        for (int i = MAX_HIGH_SCORES - 1; i > position; i--) {
            playerNames[i] = playerNames[i - 1];
            playerScores[i] = playerScores[i - 1];
        }

        // Insert the new player's name and score
        playerNames[position] = playerName;
        playerScores[position] = playerScore;

        // Save the updated high scores list
        saveHighScores();
    }
}

// Function to display high scores on game over screen
void displayHighScoresGameOver(sf::RenderWindow& window, sf::Font& font) {
    sf::Text highScoresText;
    highScoresText.setFont(font);
    highScoresText.setCharacterSize(30);
    highScoresText.setFillColor(sf::Color::White);

    std::string highScoresDisplay = "High Scores:\n";
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        if (playerScores[i] != 0) {
            highScoresDisplay += std::to_string(i + 1) + ". " + playerNames[i] + ": " + std::to_string(playerScores[i]) + "\n";
        }
    }
    highScoresText.setString(highScoresDisplay);
    highScoresText.setPosition(50.f, 50.f); // Position the text

    window.draw(highScoresText);
}

// New function to show high scores from main menu
void showHighScores(RenderWindow& window, sf::Font& font) {
    loadHighScores(); // Ensure high scores are loaded

    // Create a simple high scores screen
    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }

            // Handle key presses to return to main menu
            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Escape || event.key.code == Keyboard::Enter) {
                    return; // Return to main menu
                }
            }
        }

        window.clear();

        // Display high scores
        sf::Text highScoresText;
        highScoresText.setFont(font);
        highScoresText.setCharacterSize(30);
        highScoresText.setFillColor(sf::Color::White);

        std::string highScoresDisplay = "High Scores:\n";
        for (int i = 0; i < MAX_HIGH_SCORES; i++) {
            if (playerScores[i] != 0) {
                highScoresDisplay += std::to_string(i + 1) + ". " + playerNames[i] + ": " + std::to_string(playerScores[i]) + "\n";
            }
        }
        highScoresText.setString(highScoresDisplay);
        highScoresText.setPosition(50.f, 50.f); // Position the text

        // Add a prompt to go back
        sf::Text backText("Press ESC or ENTER to return to Main Menu", font, 20);
        backText.setFillColor(sf::Color::White);
        backText.setPosition(50.f, resolutionY - 50.f);

        window.draw(highScoresText);
        window.draw(backText);

        window.display();
    }
}

// Modified gameOver to use global font
void gameOver(sf::RenderWindow& window, int playerScore) {
    // Create the "Game Over" text
    sf::Text gameOverText("Game Over", font, 60);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setStyle(sf::Text::Bold);
    gameOverText.setPosition(window.getSize().x / 2 - gameOverText.getGlobalBounds().width / 2, window.getSize().y / 3);

    // Create the prompt text for actions (Quit, Restart, View High Scores)
    sf::Text quitText("Press Q to Quit or R to Restart", font, 30);
    quitText.setFillColor(sf::Color::White);
    quitText.setPosition(window.getSize().x / 2 - quitText.getGlobalBounds().width / 2, window.getSize().y / 2 + 50);

    // Create the high scores prompt text
    sf::Text highScoresText("Press H to View High Scores", font, 30);
    highScoresText.setFillColor(sf::Color::White);
    highScoresText.setPosition(window.getSize().x / 2 - highScoresText.getGlobalBounds().width / 2, window.getSize().y / 2 + 100);

    // Prompt user for player name in terminal
    std::string playerName = "";
    std::cout << "Enter your name: ";
    std::getline(std::cin, playerName);  // Get name from terminal input

    // Ensure player name is not empty
    if (playerName.empty()) {
        playerName = "Player";  // Default name
    }
    insertHighScore(playerName, playerScore);

    bool isViewingHighScores = false;  // Flag to check if we are viewing high scores

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Handle key events for quitting, restarting, or viewing high scores
            if (event.type == sf::Event::KeyPressed) {
                if (isViewingHighScores) {
                    // If viewing high scores, handle key events to return to the game over screen
                    if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::Enter) {
                        isViewingHighScores = false;  // Stop viewing high scores
                    }
                }
                else {
                    // Handle game over actions after typing the name
                    if (event.key.code == sf::Keyboard::R) {
                        // Restart the game
                        return; // Exit the function to restart the game
                    }
                    if (event.key.code == sf::Keyboard::Q) {
                        // Quit the game
                        window.close();
                    }
                    if (event.key.code == sf::Keyboard::H) {
                        // View high scores
                        isViewingHighScores = true;
                        loadHighScores(); // Load high scores before displaying
                    }
                }
            }
        }

        window.clear();

        if (isViewingHighScores) {
            // Display the high scores screen using the existing function
            displayHighScoresGameOver(window, font);
        }
        else {
            // Display the game over screen and prompt for actions
            window.draw(gameOverText);
            window.draw(quitText);
            window.draw(highScoresText);
        }

        window.display();
    }

    // Save the updated high scores
    saveHighScores();
}

// Function to initialize bees
void initializeBee(Bee& bee, Texture& texture, bool isKiller, int x, int y) {
    bee.sprite.setTexture(texture);
    bee.sprite.setPosition(x, y);
    bee.isKiller = isKiller;
    bee.speed = isKiller ? 10.f : 8.f;  // Killer bees move faster
}

// Function to reset flowers
void resetFlowers(Flower flowers[], int& flowerCount, int startRow, int beeX, Texture& text) {
    // Initialize flowers based on flowerCount
    for (int i = 0; i < flowerCount; i++) {
        flowers[i].sprite.setTexture(text);
        flowers[i].sprite.setScale(1.f, 1.f);
        flowers[i].sprite.setPosition(i * boxPixelsX, startRow * boxPixelsY);
    }
}

// Function to initialize hummingbird
void initializeHummingbird(Hummingbird& bird, Texture& texture) {
    bird.sprite.setTexture(texture);
    bird.sprite.setScale(1.f, 1.f);  // Scale appropriately
    bird.sprite.setPosition(0, resolutionY); // Starting position
    bird.direction = Direction(1.f, 0.f); // Start moving to the right
    bird.isAlive = false;
    bird.isWaiting = false;
    bird.sicknessCount = 0;
}

// Function to display score and lives
void displayScoreAndLives(sf::RenderWindow& window, int score, int lives, int sprayCount) {
    // Create score and lives text objects
    sf::Text scoreText;
    sf::Text livesText;
    sf::Text sprayText;

    // Set the font, size, and color
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);

    livesText.setFont(font);
    livesText.setCharacterSize(24);
    livesText.setFillColor(sf::Color::White);

    sprayText.setFont(font);
    sprayText.setCharacterSize(24);
    sprayText.setFillColor(sf::Color::White);

    // Set the strings for score and lives
    scoreText.setString("Score: " + std::to_string(score));
    livesText.setString("Lives: " + std::to_string(lives));
    sprayText.setString("Sprays: " + std::to_string(sprayCount));

    // Position the score and lives at the bottom left corner
    scoreText.setPosition(20, resolutionY - 60);  // Adjusted positions
    livesText.setPosition(20, resolutionY - 30);
    sprayText.setPosition(150, resolutionY - 45);

    // Draw the score and lives text on the window
    window.draw(scoreText);
    window.draw(livesText);
    window.draw(sprayText);
}

// Function to check collision between player and any flower
bool checkPlayerCollisionWithFlowers(const Sprite& player, const Flower flowers[], int flowerCount) {
    for (int i = 0; i < flowerCount; i++) {
        if (player.getGlobalBounds().intersects(flowers[i].sprite.getGlobalBounds())) {
            return true;
        }
    }
    return false;
}

// Game for Level 1
void level1Game(RenderWindow& window) {
    // Load textures
    Texture playerTexture, beeTexture, killerBeeTexture, honeycombTexture, hummingbirdTexture, bulletTexture, flowerTexture;
    if (!playerTexture.loadFromFile("Textures/spray.png") ||
        !flowerTexture.loadFromFile("Textures/obstacles.png") ||
        !beeTexture.loadFromFile("Textures/Regular_bee.png") ||
        !killerBeeTexture.loadFromFile("Textures/Fast_bee.png") ||
        !honeycombTexture.loadFromFile("Textures/honeycomb.png") ||
        !hummingbirdTexture.loadFromFile("Textures/bird.png", sf::IntRect(0, 0, 32, 32)) ||
        !bulletTexture.loadFromFile("Textures/bullet.png")) {
        cout << "Error: Could not load one or more textures!" << endl;
        return;
    }

    // Player setup
    Sprite player(playerTexture);
    int flowerRowStart = resolutionY / boxPixelsY - 2;
    player.setPosition(resolutionX / 2 - boxPixelsX / 2, (flowerRowStart - 1) * boxPixelsY);

    // Bees setup using fixed array as per user's code
    Bee bees[20];
    int beeCount = 0;
    int deathCount = 0;
    Clock beeClock, beeGenClock;

    // Flowers setup
    Flower flowers[31];
    int flowerCount = 3; // Initialize to 3 since already added 3 honeycombs
    int flowerCountLeft = 0, flowerCountRight = 0;
    resetFlowers(flowers, flowerCount, flowerRowStart, 0, flowerTexture);

    // HummingBird
    Hummingbird bird;
    bird.birdAppearanceTimer.restart();

    initializeHummingbird(bird, hummingbirdTexture);

    // Bullet setup
    float bullet_x, bullet_y;
    bool bulletExists = false;
    Clock bulletClock;
    Sprite bulletSprite(bulletTexture);
    bulletSprite.setScale(2.5, 2.5);

    Honeycomb honeycombs[30];
    int honeycombCount = 0;

    for (int i = 0; i < 3; i++) {
        honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
        honeycombs[honeycombCount].sprite.setColor(sf::Color::Red);
        honeycombs[honeycombCount].sprite.setPosition(rand() % (resolutionX - 70), rand() % (resolutionY - 100));
        honeycombs[honeycombCount].honeycomb_collection.restart();
        honeycombCount++;
    }

    while (window.isOpen()) {
        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) window.close();
        }

        // -------------------
        // Player Movement with Collision Detection
        // -------------------
        // Move Left
        if (Keyboard::isKeyPressed(Keyboard::Left) && player.getPosition().x > 0) {
            // Calculate the new position
            Sprite tempPlayer = player;
            tempPlayer.move(-0.5f, 0);

            // Check collision with any flower
            bool collision = false;
            for (int i = 0; i < flowerCount; i++) {
                if (tempPlayer.getGlobalBounds().intersects(flowers[i].sprite.getGlobalBounds())) {
                    collision = true;
                    break;
                }
            }
           // if (!collision) {
                player.move(-0.5f, 0);
           // }
        }

        // Move Right
        if (Keyboard::isKeyPressed(Keyboard::Right) && player.getPosition().x < resolutionX - boxPixelsX) {
            // Calculate the new position
            Sprite tempPlayer = player;
            tempPlayer.move(0.5f, 0);

            // Check collision with any flower
            bool collision = false;
            for (int i = 0; i < flowerCount; i++) {
                if (tempPlayer.getGlobalBounds().intersects(flowers[i].sprite.getGlobalBounds())) {
                    collision = true;
                    break;
                }
            }
           // if (!collision) {
                player.move(0.5f, 0);
            //}
        }

        if (Keyboard::isKeyPressed(Keyboard::X))
            exit(0);

        // Shooting bullets
        if (Keyboard::isKeyPressed(Keyboard::Space) && !bulletExists && sprayCount > 0) {
            bullet_x = player.getPosition().x + boxPixelsX / 4;
            bullet_y = player.getPosition().y;
            bulletExists = true;
            sprayCount--;
        }

        // Bee Creation
        if (beeGenClock.getElapsedTime().asMilliseconds() > 1500 && beeCount < maxBees) {
            beeGenClock.restart();
            initializeBee(bees[beeCount], beeTexture, false, 50, 50);
            beeCount++;
        }

        // HummingBird Behavior
        if (bird.isAlive) {
            if (bird.isWaiting) {
                // Wait for 3 seconds before deciding the direction
                if (bird.moveTimer.getElapsedTime().asSeconds() > 3.f) {
                    bird.isWaiting = false; // End the decision phase
                    // Pick a random direction: -0.5f, 0, or 0.5f for both x and y
                    bird.direction = Direction((rand() % 3 - 1) * 0.5f, (rand() % 3 - 1) * 0.5f);
                    bird.moveTimer.restart(); // Restart the timer for movement
                }
            }
            else {
                // Move the bird
                if (bird.moveTimer.getElapsedTime().asSeconds() > 2.f) { // Move for 2 seconds
                    bird.isWaiting = true; // Enter decision phase
                    bird.moveTimer.restart(); // Restart the timer for waiting
                }

                Direction pos( bird.sprite.getPosition().x, bird.sprite.getPosition().y);
                pos.x += bird.direction.x * 0.15f; // Adjust movement speed if needed
                pos.y += bird.direction.y * 0.15f;
                // Wrap around screen edges
                if (pos.x < 0) pos.x = resolutionX;
                if (pos.x > resolutionX) pos.x = 0;
                if (pos.y < 0) pos.y = resolutionY;
                if (pos.y > resolutionY) pos.y = 0;

                bird.sprite.setPosition(pos.x, pos.y);
            }
        }

        // Hummingbird Appearance
        if (bird.birdAppearanceTimer.getElapsedTime().asSeconds() >= 20.f &&
            bird.birdAppearanceTimer.getElapsedTime().asSeconds() <= 30.f) {
            if (!bird.isAlive) {
                bird.isAlive = true;
                bird.sprite.setPosition(0, resolutionY);
            }
        }

        // Lives Management
        if (sprayCount == 0) {
            if (lives > 0) {
                lives--;
                sprayCount = 56;
            }
            else {
                cout << "GameOver" << endl;
                gameOver(window, score);
            }
        }

        if (flowerCount >= maxFlowers) {
            cout << "GameOver" << endl;
            gameOver(window, score);
        }

        if (deathCount == 20)
            return;

        // Bee Collision with honeycombs
        for (int i = 0; i < beeCount; i++) {
            if (bees[i].isAlive) {
                // Check collision with honeycombs for worker bees
                if (!bees[i].isKiller) { // Worker bee
                    for (int j = 0; j < honeycombCount; j++) {
                        if (!honeycombs[j].isCollected && bees[i].sprite.getGlobalBounds().intersects(honeycombs[j].sprite.getGlobalBounds())) {
                            // Worker bee collides with honeycomb
                            Direction pos ( bees[i].sprite.getPosition().x, bees[i].sprite.getPosition().y);
                            pos.y += boxPixelsY;
                            bees[i].sprite.setPosition(pos.x, pos.y);
                            score += 50;                     // Award points for collision
                            break;
                        }
                    }
                }
            }
        }

        // Bullet movement
        if (bulletExists) {
            bullet_y -= 0.5;
            bulletSprite.setPosition(bullet_x, bullet_y);

            if (bullet_y < 0) {
                bulletExists = false;
            }

            // Handle collisions with bees
            for (int i = 0; i < beeCount; i++) {
                if (bees[i].isAlive && bees[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y)) {
                    bees[i].isAlive = false;
                    deathCount++;

                    if (honeycombCount < maxHoneycombs) {
                        honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
                        if (bees[i].isKiller)
                            honeycombs[honeycombCount].sprite.setColor(sf::Color::Red);
                        honeycombs[honeycombCount].sprite.setPosition(bees[i].sprite.getPosition());
                        honeycombs[honeycombCount].honeycomb_collection.restart();
                        honeycombCount++;
                    }
                    bulletExists = false;
                    score += bees[i].isKiller ? 200 : 100;
                }
            }

            for (int i = 0; i < honeycombCount; i++) {
                if (!honeycombs[i].isCollected &&
                    honeycombs[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y) && honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 1.f) {
                    honeycombs[i].isCollected = true; // Mark the honeycomb as collected
                    bulletExists = false;            // Destroy the bullet
                    score += 50;                     // Award points for destroying a honeycomb
                    break; // Exit the loop since the bullet can only collide with one honeycomb at a time
                }
            }
        }

        // Move bees linearly
        if (beeClock.getElapsedTime().asMilliseconds() > 50) {
            beeClock.restart();
            for (int i = 0; i < beeCount; i++) {
                if (bees[i].isAlive) {
                    Direction pos(bees[i].sprite.getPosition().x, bees[i].sprite.getPosition().y);

                    // Worker bee random stop logic
                    if (!bees[i].isKiller) { // Only for worker bees
                        // Randomly decide to stop (1% chance per frame)
                        if (!bees[i].isStopped && rand() % 100 < 1) {
                            bees[i].isStopped = true;
                            bees[i].stopTimer.restart(); // Start the stop timer
                        }

                        // Resume movement after stopping for 2 seconds
                        if (bees[i].isStopped && bees[i].stopTimer.getElapsedTime().asSeconds() > 2) {
                            bees[i].isStopped = false; // Resume movement
                        }
                    }

                    // Handle pollination and exiting
                    if (!bees[i].isStopped) { // Only move if the bee is not stopped
                        if (pos.y >= resolutionY - (4 * boxPixelsY) && !bees[i].isExiting) {
                            bees[i].isExiting = true; // Mark for exit after pollination

                            // Determine the side of the screen where the bee is
                            if (pos.x > resolutionX / 2) { // Right side
                                if (flowerCount < maxFlowers) {
                                    if (flowerCountRight == 0) {
                                        flowers[flowerCount].sprite.setTexture(flowerTexture);
                                        flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                        flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
                                        flowerCountRight++;
                                        flowerCount++;
                                    }
                                    flowers[flowerCount].sprite.setTexture(flowerTexture);
                                    flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                    flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
                                    flowerCount++;
                                    flowerCountRight++;
                                    deathCount++;
                                }
                            }
                            else { // Left side
                                if (flowerCount < maxFlowers) {
                                    if (flowerCountLeft == 0) {
                                        flowers[flowerCount].sprite.setTexture(flowerTexture);
                                        flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                        flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
                                        flowerCountLeft++;
                                        flowerCount++;
                                    }
                                    flowers[flowerCount].sprite.setTexture(flowerTexture);
                                    flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                    flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
                                    flowerCount++;
                                    flowerCountLeft++;
                                    deathCount++;
                                }
                            }
                        }

                        // Handle exiting bees
                        if (bees[i].isExiting) {
                            if (bees[i].sprite.getPosition().x > resolutionX / 2)
                                pos.x += bees[i].speed;
                            else
                                pos.x -= bees[i].speed;
                            if (pos.y > resolutionY) {
                                bees[i].isAlive = false; // Remove bee once it exits the screen
                            }
                        }
                        else {
                            // Normal movement logic
                            pos.x += bees[i].beeDirection * bees[i].speed;
                            if (pos.x >= resolutionX - boxPixelsX || pos.x <= 0) {
                                pos.y += boxPixelsY; // Drop a tier when hitting the screen edge
                                bees[i].beeDirection = -bees[i].beeDirection;
                            }
                        }
                    }

                    bees[i].sprite.setPosition(pos.x, pos.y);
                }
            }
        }

        // Eating Honeycombs
        for (int i = 0; i < honeycombCount; i++) {
            if (!honeycombs[i].isCollected &&
                bird.sprite.getGlobalBounds().intersects(honeycombs[i].sprite.getGlobalBounds()) && !honeycombs[i].collection_start) {

                honeycombs[i].collection_start = true;
                honeycombs[i].honeycomb_collection.restart();
                if (honeycombs[i].sprite.getColor() == sf::Color::Red) {
                    int row = honeycombs[i].sprite.getPosition().y / rowHeight; // Get the Y-coordinate and divide by the row height

                    int points = 1500; // Default points for remaining rows
                    if (row == 0 || row == 1) {
                        points = 2000; // Top two rows
                    }
                    else if (row == 2 || row == 3 || row == 4) {
                        points = 1800; // Third, fourth, and fifth rows
                    }
                    score += points;
                    cout << "Red Honeycomb" << endl;
                }

                else {
                    int row = honeycombs[i].sprite.getPosition().y / rowHeight; // Get the Y-coordinate and divide by the row height

                    int points = 500; // Default points for remaining rows
                    if (row == 0 || row == 1) {
                        points = 1000; // Top two rows
                    }
                    else if (row == 2 || row == 3 || row == 4) {
                        points = 800; // Third, fourth, and fifth rows

                        cout << "Yellow Honeycomb" << endl;
                    }
                    score += points;
                }

                cout << "Score: " << score << endl;


                // Pause the bird after eating the honeycomb
                bird.isPausedAfterHoneycomb = true;
                bird.isWaiting = true; // Start waiting phase
                bird.waitTimer.restart(); // Reset waiting timer
            }
        }

        // Disappearing Honeycomb
        for (int i = 0; i < honeycombCount; i++) {
            if (honeycombs[i].collection_start && honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 2.f) {
                honeycombs[i].isCollected = true;
            }
        }

        // Sickness
        if (bulletExists && bird.sprite.getGlobalBounds().contains(bullet_x, bullet_y)) {
            bird.sicknessCount++;
            bulletExists = false; // Remove the bullet

            if (bird.sicknessCount >= 3) {
                bird.isAlive = false;
                bird.sicknessCount = 0;
                bird.respawnTimer.restart(); // Start respawn timer
            }
        }

        // Render
        window.clear();
        window.draw(player);

        // HUD
        displayScoreAndLives(window, score, lives, sprayCount);
        if (bulletExists) {
            window.draw(bulletSprite);
        }

        for (int i = 0; i < beeCount; i++) {
            if (bees[i].isAlive) window.draw(bees[i].sprite);
        }

        for (int i = 0; i < honeycombCount; i++) {
            if (!honeycombs[i].isCollected) window.draw(honeycombs[i].sprite);
        }

        for (int i = 0; i < flowerCount; i++) {
            window.draw(flowers[i].sprite);
        }

        // Draw HummingBird
        if (bird.isAlive)
            window.draw(bird.sprite);

        window.display();
    }
}

    // Game for Level 2
void level2Game(RenderWindow& window) {
    // Increase difficulty: more bees, more honeycombs, etc.
    // Load textures
    Texture playerTexture, beeTexture, killerBeeTexture, honeycombTexture, hummingbirdTexture, bulletTexture, flowerTexture;
    if (!playerTexture.loadFromFile("Textures/spray.png") ||
        !flowerTexture.loadFromFile("Textures/obstacles.png") ||
        !beeTexture.loadFromFile("Textures/Regular_bee.png") ||
        !killerBeeTexture.loadFromFile("Textures/Fast_bee.png") ||
        !honeycombTexture.loadFromFile("Textures/honeycomb.png") ||
        !hummingbirdTexture.loadFromFile("Textures/bird.png", sf::IntRect(0, 0, 32, 32)) ||
        !bulletTexture.loadFromFile("Textures/bullet.png")) {
        cout << "Error: Could not load one or more textures!" << endl;
        return;
    }

    // Player setup (reuse global variables)
    Sprite player(playerTexture);
    int flowerRowStart = resolutionY / boxPixelsY - 2;
    player.setPosition(resolutionX / 2 - boxPixelsX / 2, (flowerRowStart - 1) * boxPixelsY);

    // Bees setup with increased maxBees for difficulty
    Bee bees[25];
    int beeCount = 0;
    int killerBeeCount = 0;
    int deathCount = 0;
    Clock beeClock, beeGenClock;

    // Flowers setup
    Flower flowers[31];
    int flowerCount = 0; // Start with no flowers
    int flowerCountLeft = 0, flowerCountRight = 0;
    resetFlowers(flowers, flowerCount, flowerRowStart, 0, flowerTexture);

    // HummingBird
    Hummingbird bird;
    bird.birdAppearanceTimer.restart();

    initializeHummingbird(bird, hummingbirdTexture);

    // Bullet setup
    float bullet_x, bullet_y;
    bool bulletExists = false;
    Clock bulletClock;
    Sprite bulletSprite(bulletTexture);
    bulletSprite.setScale(2.5, 2.5);

    Honeycomb honeycombs[30];
    int honeycombCount = 0;

    // Initialize 5 honeycombs at random positions
    for (int i = 0; i < 5; i++) {
        honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
        honeycombs[honeycombCount].sprite.setColor(sf::Color::Red);
        honeycombs[honeycombCount].sprite.setPosition(rand() % (resolutionX - 70), rand() % (resolutionY - 100));
        honeycombs[honeycombCount].honeycomb_collection.restart();
        honeycombCount++;
    }

    while (window.isOpen()) {
        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) window.close();
        }

        // -------------------
        // Player Movement with Collision Detection
        // -------------------
        // Move Left
        if (Keyboard::isKeyPressed(Keyboard::Left) && player.getPosition().x > 0) {
            // Calculate the new position
            Sprite tempPlayer = player;
            tempPlayer.move(-0.5f, 0);

            // Check collision with any flower
            bool collision = false;
            for (int i = 0; i < flowerCount; i++) {
                if (tempPlayer.getGlobalBounds().intersects(flowers[i].sprite.getGlobalBounds())) {
                    collision = true;
                    break;
                }
            }
      //      if (!collision) {
                player.move(-0.5f, 0);
       //     }
        }

        // Move Right
        if (Keyboard::isKeyPressed(Keyboard::Right) && player.getPosition().x < resolutionX - boxPixelsX) {
            // Calculate the new position
            Sprite tempPlayer = player;
            tempPlayer.move(0.5f, 0);

            // Check collision with any flower
            bool collision = false;
            for (int i = 0; i < flowerCount; i++) {
                if (tempPlayer.getGlobalBounds().intersects(flowers[i].sprite.getGlobalBounds())) {
                    collision = true;
                    break;
                }
            }
          //  if (!collision) {
                player.move(0.5f, 0);
           // }
        }

        if (Keyboard::isKeyPressed(Keyboard::X))
            exit(0);

        // Shooting bullets
        if (Keyboard::isKeyPressed(Keyboard::Space) && !bulletExists && sprayCount > 0) {
            bullet_x = player.getPosition().x + boxPixelsX / 4;
            bullet_y = player.getPosition().y;
            bulletExists = true;
            sprayCount--;
        }

        // Bee Creation with increased difficulty
        if (beeGenClock.getElapsedTime().asMilliseconds() > 1200 && beeCount < maxBees) {
            beeGenClock.restart();
            int type = rand() % 10;
            if (type > 7 && killerBeeCount < 7) {
                initializeBee(bees[beeCount], killerBeeTexture, true, 50, 50);
                beeCount++;
                killerBeeCount++;
            }
            else {
                initializeBee(bees[beeCount], beeTexture, false, 50, 50);
                beeCount++;
            }
        }

        // HummingBird Behavior
        if (bird.isAlive) {
            if (bird.isWaiting) {
                // Wait for 3 seconds before deciding the direction
                if (bird.moveTimer.getElapsedTime().asSeconds() > 3.f) {
                    bird.isWaiting = false; // End the decision phase
                    // Pick a random direction: -0.5f, 0, or 0.5f for both x and y
                    bird.direction = Direction((rand() % 3 - 1) * 0.5f, (rand() % 3 - 1) * 0.5f);
                    bird.moveTimer.restart(); // Restart the timer for movement
                }
            }
            else {
                // Move the bird
                if (bird.moveTimer.getElapsedTime().asSeconds() > 2.f) { // Move for 2 seconds
                    bird.isWaiting = true; // Enter decision phase
                    bird.moveTimer.restart(); // Restart the timer for waiting
                }

// Convert the position to Direction (for easier manipulation)
Direction pos(bird.sprite.getPosition().x, bird.sprite.getPosition().y);

// Manually add the direction to the position (multiply by scalar)
pos.x += bird.direction.x * 0.15f;
pos.y += bird.direction.y * 0.15f;
                // Wrap around screen edges
                if (pos.x < 0) pos.x = resolutionX;
                if (pos.x > resolutionX) pos.x = 0;
                if (pos.y < 0) pos.y = resolutionY;
                if (pos.y > resolutionY) pos.y = 0;

                bird.sprite.setPosition(pos.x, pos.y);
            }
        }

        // Hummingbird Appearance
        if (bird.birdAppearanceTimer.getElapsedTime().asSeconds() >= 20.f &&
            bird.birdAppearanceTimer.getElapsedTime().asSeconds() <= 30.f) {
            if (!bird.isAlive) {
                bird.isAlive = true;
                bird.sprite.setPosition(0, resolutionY);
            }
        }

        // Lives Management
        if (sprayCount == 0) {
            if (lives > 0) {
                lives--;
                sprayCount = 56;
            }
            else {
                cout << "GameOver" << endl;
                gameOver(window, score);
            }
        }

        if (deathCount == 20)
            return;
        cout << deathCount << endl;
        // Flower Count Management
        if (flowerCount >= maxFlowers) {
            cout << "GameOver" << endl;
            gameOver(window, score);
        }

        // Bee Collision with honeycombs
        for (int i = 0; i < beeCount; i++) {
            if (bees[i].isAlive) {
                // Check collision with honeycombs for worker bees
                if (!bees[i].isKiller) { // Worker bee
                    for (int j = 0; j < honeycombCount; j++) {
                        if (!honeycombs[j].isCollected && bees[i].sprite.getGlobalBounds().intersects(honeycombs[j].sprite.getGlobalBounds())) {
                            // Worker bee collides with honeycomb
                            Direction pos ( bees[i].sprite.getPosition().x, bees[i].sprite.getPosition().y);
                            pos.y += boxPixelsY;
                            bees[i].sprite.setPosition(pos.x, pos.y);
                            score += 50;                     // Award points for collision
                            break;
                        }
                    }
                }
            }
        }

        // Bullet movement
        if (bulletExists) {
            bullet_y -= 0.5;
            bulletSprite.setPosition(bullet_x, bullet_y);

            if (bullet_y < 0) {
                bulletExists = false;
            }

            // Handle collisions with bees
            for (int i = 0; i < beeCount; i++) {
                if (bees[i].isAlive && bees[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y)) {
                    bees[i].isAlive = false;
                    deathCount++;

                    if (honeycombCount < maxHoneycombs) {
                        honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
                        if (bees[i].isKiller)
                            honeycombs[honeycombCount].sprite.setColor(sf::Color::Red);
                        honeycombs[honeycombCount].sprite.setPosition(bees[i].sprite.getPosition());
                        honeycombs[honeycombCount].honeycomb_collection.restart();
                        honeycombCount++;
                    }
                    bulletExists = false;
                    score += bees[i].isKiller ? 200 : 100;
                    cout << "Display Score: " << score << endl;
                }
            }

            // Handle collisions with honeycombs
            for (int i = 0; i < honeycombCount; i++) {
                if (!honeycombs[i].isCollected &&
                    honeycombs[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y) &&
                    honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 1.f) {
                    honeycombs[i].isCollected = true; // Mark the honeycomb as collected
                    bulletExists = false;            // Destroy the bullet
                    score += 50;                     // Award points for destroying a honeycomb
                    break; // Exit the loop since the bullet can only collide with one honeycomb at a time
                }
            }
        }

        // Move bees linearly
        if (beeClock.getElapsedTime().asMilliseconds() > 50) {
            beeClock.restart();
            for (int i = 0; i < beeCount; i++) {
                if (bees[i].isAlive) {
                    Direction pos ( bees[i].sprite.getPosition().x, bees[i].sprite.getPosition().y);

                    // Worker bee random stop logic
                    if (!bees[i].isKiller) { // Only for worker bees
                        // Randomly decide to stop (1% chance per frame)
                        if (!bees[i].isStopped && rand() % 100 < 1) {
                            bees[i].isStopped = true;
                            bees[i].stopTimer.restart(); // Start the stop timer
                        }

                        // Resume movement after stopping for 2 seconds
                        if (bees[i].isStopped && bees[i].stopTimer.getElapsedTime().asSeconds() > 2) {
                            bees[i].isStopped = false; // Resume movement
                        }
                    }

                    // Handle pollination and exiting
                    if (!bees[i].isStopped) { // Only move if the bee is not stopped
                        if (pos.y >= resolutionY - (4 * boxPixelsY) && !bees[i].isExiting) {
                            bees[i].isExiting = true; // Mark for exit after pollination

                            // Determine the side of the screen where the bee is
                            if (pos.x > resolutionX / 2) { // Right side
                                if (flowerCount < maxFlowers) {
                                    if (flowerCountRight == 0) {
                                        flowers[flowerCount].sprite.setTexture(flowerTexture);
                                        flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                        flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
                                        flowerCountRight++;
                                        flowerCount++;
                                    }
                                    flowers[flowerCount].sprite.setTexture(flowerTexture);
                                    flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                    flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
                                    flowerCount++;
                                    flowerCountRight++;
                                    deathCount++;
                                }
                            }
                            else { // Left side
                                if (flowerCount < maxFlowers) {
                                    if (flowerCountLeft == 0) {
                                        flowers[flowerCount].sprite.setTexture(flowerTexture);
                                        flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                        flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
                                        flowerCountLeft++;
                                        flowerCount++;
                                    }
                                    flowers[flowerCount].sprite.setTexture(flowerTexture);
                                    flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                    flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
                                    flowerCount++;
                                    flowerCountLeft++;
                                    deathCount++;
                                }
                            }
                        }

                        // Handle exiting bees
                        if (bees[i].isExiting) {
                            if (bees[i].sprite.getPosition().x > resolutionX / 2)
                                pos.x += bees[i].speed;
                            else
                                pos.x -= bees[i].speed;
                            if (pos.y > resolutionY) {
                                bees[i].isAlive = false; // Remove bee once it exits the screen
                            }
                        }
                        else {
                            // Normal movement logic
                            pos.x += bees[i].beeDirection * bees[i].speed;
                            if (pos.x >= resolutionX - boxPixelsX || pos.x <= 0) {
                                pos.y += boxPixelsY; // Drop a tier when hitting the screen edge
                                bees[i].beeDirection = -bees[i].beeDirection;
                            }
                        }
                    }

                    bees[i].sprite.setPosition(pos.x, pos.y);
                }
            }
        }

        // Eating Honeycombs
        for (int i = 0; i < honeycombCount; i++) {
            if (!honeycombs[i].isCollected &&
                bird.sprite.getGlobalBounds().intersects(honeycombs[i].sprite.getGlobalBounds()) && !honeycombs[i].collection_start) {

                honeycombs[i].collection_start = true;
                honeycombs[i].honeycomb_collection.restart();

                if (honeycombs[i].sprite.getColor() == sf::Color::Red) {
                    int row = honeycombs[i].sprite.getPosition().y / rowHeight; // Get the Y-coordinate and divide by the row height

                    int points = 1500; // Default points for remaining rows
                    if (row == 0 || row == 1) {
                        points = 2000; // Top two rows
                    }
                    else if (row == 2 || row == 3 || row == 4) {
                        points = 1800; // Third, fourth, and fifth rows
                    }
                    score += points;
                    cout << "Red Honeycomb" << endl;
                }

                else {
                    int row = honeycombs[i].sprite.getPosition().y / rowHeight; // Get the Y-coordinate and divide by the row height

                    int points = 500; // Default points for remaining rows
                    if (row == 0 || row == 1) {
                        points = 1000; // Top two rows
                    }
                    else if (row == 2 || row == 3 || row == 4) {
                        points = 800; // Third, fourth, and fifth rows

                        cout << "Yellow Honeycomb" << endl;
                    }
                    score += points;
                }

                cout << "Score: " << score << endl;


                // Pause the bird after eating the honeycomb
                bird.isPausedAfterHoneycomb = true;
                bird.isWaiting = true; // Start waiting phase
                bird.waitTimer.restart(); // Reset waiting timer
            }
        }

        // Disappearing Honeycomb
        for (int i = 0; i < honeycombCount; i++) {
            if (honeycombs[i].collection_start && honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 2.f) {
                honeycombs[i].isCollected = true;
            }
        }

        // Sickness
        if (bulletExists && bird.sprite.getGlobalBounds().contains(bullet_x, bullet_y)) {
            bird.sicknessCount++;
            bulletExists = false; // Remove the bullet

            if (bird.sicknessCount >= 3) {
                bird.isAlive = false;
                bird.sicknessCount = 0;
                bird.respawnTimer.restart(); // Start respawn timer
            }
        }

        // Render
        window.clear();
        window.draw(player);

        // HUD
        displayScoreAndLives(window, score, lives, sprayCount);
        if (bulletExists) {
            window.draw(bulletSprite);
        }

        for (int i = 0; i < beeCount; i++) {
            if (bees[i].isAlive) window.draw(bees[i].sprite);
        }

        for (int i = 0; i < honeycombCount; i++) {
            if (!honeycombs[i].isCollected) window.draw(honeycombs[i].sprite);
        }

        for (int i = 0; i < flowerCount; i++) {
            window.draw(flowers[i].sprite);
        }

        // Draw HummingBird
        if (bird.isAlive)
            window.draw(bird.sprite);

        window.display();
    }
}
        // Game for Level 2
        //void level2Game(RenderWindow & window) {
        //    // Similar to level1Game, but with increased difficulty
        //    // You can customize this function as needed for Level 2

        //    // Load textures
        //    Texture playerTexture, beeTexture, killerBeeTexture, honeycombTexture, hummingbirdTexture, bulletTexture, flowerTexture;
        //    if (!playerTexture.loadFromFile("Textures/spray.png") ||
        //        !flowerTexture.loadFromFile("Textures/obstacles.png") ||
        //        !beeTexture.loadFromFile("Textures/Regular_bee.png") ||
        //        !killerBeeTexture.loadFromFile("Textures/Fast_bee.png") ||
        //        !honeycombTexture.loadFromFile("Textures/honeycomb.png") ||
        //        !hummingbirdTexture.loadFromFile("Textures/bird.png", sf::IntRect(0, 0, 32, 32)) ||
        //        !bulletTexture.loadFromFile("Textures/bullet.png")) {
        //        cout << "Error: Could not load one or more textures!" << endl;
        //        return;
        //    }

        //    // Player setup (reuse global variables)
        //    Sprite player(playerTexture);
        //    int flowerRowStart = resolutionY / boxPixelsY - 2;
        //    player.setPosition(resolutionX / 2 - boxPixelsX / 2, (flowerRowStart - 1) * boxPixelsY);

        //    // Bees setup with increased maxBees for difficulty
        //    Bee bees[25];
        //    int beeCount = 0;
        //    int killerBeeCount = 0;
        //    int deathCount = 0;
        //    Clock beeClock, beeGenClock;

        //    // Flowers setup
        //    Flower flowers[31];
        //    int flowerCount = 0; // Start with no flowers
        //    int flowerCountLeft = 0, flowerCountRight = 0;
        //    resetFlowers(flowers, flowerCount, flowerRowStart, 0, flowerTexture);

        //    // HummingBird
        //    Hummingbird bird;
        //    bird.birdAppearanceTimer.restart();

        //    initializeHummingbird(bird, hummingbirdTexture);

        //    // Bullet setup
        //    float bullet_x, bullet_y;
        //    bool bulletExists = false;
        //    Clock bulletClock;
        //    Sprite bulletSprite(bulletTexture);
        //    bulletSprite.setScale(2.5, 2.5);

        //    Honeycomb honeycombs[30];
        //    int honeycombCount = 0;

        //    // Initialize 5 honeycombs at random positions
        //    for (int i = 0; i < 5; i++) {
        //        honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
        //        honeycombs[honeycombCount].sprite.setColor(sf::Color::Red);
        //        honeycombs[honeycombCount].sprite.setPosition(rand() % (resolutionX - 70), rand() % (resolutionY - 100));
        //        honeycombs[honeycombCount].honeycomb_collection.restart();
        //        honeycombCount++;
        //    }

        //    while (window.isOpen()) {
        //        Event e;
        //        while (window.pollEvent(e)) {
        //            if (e.type == Event::Closed) window.close();
        //        }

        //        // -------------------
        //        // Player Movement with Collision Detection
        //        // -------------------
        //        // Move Left
        //        if (Keyboard::isKeyPressed(Keyboard::Left) && player.getPosition().x > 0) {
        //            // Calculate the new position
        //            Sprite tempPlayer = player;
        //            tempPlayer.move(-0.5f, 0);

        //            // Check collision with any flower
        //            bool collision = false;
        //            for (int i = 0; i < flowerCount; i++) {
        //                if (tempPlayer.getGlobalBounds().intersects(flowers[i].sprite.getGlobalBounds())) {
        //                    collision = true;
        //                    break;
        //                }
        //            }
        //            if (!collision) {
        //                player.move(-0.5f, 0);
        //            }
        //        }

        //        // Move Right
        //        if (Keyboard::isKeyPressed(Keyboard::Right) && player.getPosition().x < resolutionX - boxPixelsX) {
        //            // Calculate the new position
        //            Sprite tempPlayer = player;
        //            tempPlayer.move(0.5f, 0);

        //            // Check collision with any flower
        //            bool collision = false;
        //            for (int i = 0; i < flowerCount; i++) {
        //                if (tempPlayer.getGlobalBounds().intersects(flowers[i].sprite.getGlobalBounds())) {
        //                    collision = true;
        //                    break;
        //                }
        //            }
        //            if (!collision) {
        //                player.move(0.5f, 0);
        //            }
        //        }

        //        if (Keyboard::isKeyPressed(Keyboard::X))
        //            exit(0);

        //        // Shooting bullets
        //        if (Keyboard::isKeyPressed(Keyboard::Space) && !bulletExists && sprayCount > 0) {
        //            bullet_x = player.getPosition().x + boxPixelsX / 4;
        //            bullet_y = player.getPosition().y;
        //            bulletExists = true;
        //            sprayCount--;
        //        }

        //        // Bee Creation with increased difficulty
        //        if (beeGenClock.getElapsedTime().asMilliseconds() > 1200 && beeCount < maxBees) {
        //            beeGenClock.restart();
        //            int type = rand() % 10;
        //            if (type > 7 && killerBeeCount < 7) {
        //                initializeBee(bees[beeCount], killerBeeTexture, true, 50, 50);
        //                beeCount++;
        //                killerBeeCount++;
        //            }
        //            else {
        //                initializeBee(bees[beeCount], beeTexture, false, 50, 50);
        //                beeCount++;
        //            }
        //        }

        //        // HummingBird Behavior
        //        if (bird.isAlive) {
        //            if (bird.isWaiting) {
        //                // Wait for 3 seconds before deciding the direction
        //                if (bird.moveTimer.getElapsedTime().asSeconds() > 3.f) {
        //                    bird.isWaiting = false; // End the decision phase
        //                    // Pick a random direction: -0.5f, 0, or 0.5f for both x and y
        //                    bird.direction = Vector2f((rand() % 3 - 1) * 0.5f, (rand() % 3 - 1) * 0.5f);
        //                    bird.moveTimer.restart(); // Restart the timer for movement
        //                }
        //            }
        //            else {
        //                // Move the bird
        //                if (bird.moveTimer.getElapsedTime().asSeconds() > 2.f) { // Move for 2 seconds
        //                    bird.isWaiting = true; // Enter decision phase
        //                    bird.moveTimer.restart(); // Restart the timer for waiting
        //                }

        //                Vector2f pos = bird.sprite.getPosition();
        //                pos += bird.direction * 0.15f; // Adjust movement speed if needed

        //                // Wrap around screen edges
        //                if (pos.x < 0) pos.x = resolutionX;
        //                if (pos.x > resolutionX) pos.x = 0;
        //                if (pos.y < 0) pos.y = resolutionY;
        //                if (pos.y > resolutionY) pos.y = 0;

        //                bird.sprite.setPosition(pos);
        //            }
        //        }

        //        // Hummingbird Appearance
        //        if (bird.birdAppearanceTimer.getElapsedTime().asSeconds() >= 20.f &&
        //            bird.birdAppearanceTimer.getElapsedTime().asSeconds() <= 30.f) {
        //            if (!bird.isAlive) {
        //                bird.isAlive = true;
        //                bird.sprite.setPosition(0, resolutionY);
        //            }
        //        }

        //        // Lives Management
        //        if (sprayCount == 0) {
        //            if (lives > 0) {
        //                lives--;
        //                sprayCount = 56;
        //            }
        //            else {
        //                cout << "GameOver" << endl;
        //                gameOver(window, score);
        //            }
        //        }

        //        // Flower Count Management
        //        if (flowerCount >= maxFlowers) {
        //            cout << "GameOver" << endl;
        //            gameOver(window, score);
        //        }

        //        // Bee Collision with honeycombs
        //        for (int i = 0; i < beeCount; i++) {
        //            if (bees[i].isAlive) {
        //                // Check collision with honeycombs for worker bees
        //                if (!bees[i].isKiller) { // Worker bee
        //                    for (int j = 0; j < honeycombCount; j++) {
        //                        if (!honeycombs[j].isCollected && bees[i].sprite.getGlobalBounds().intersects(honeycombs[j].sprite.getGlobalBounds())) {
        //                            // Worker bee collides with honeycomb
        //                            Vector2f pos = bees[i].sprite.getPosition();
        //                            pos.y += boxPixelsY;
        //                            bees[i].sprite.setPosition(pos);
        //                            score += 50;                     // Award points for collision
        //                            break;
        //                        }
        //                    }
        //                }
        //            }
        //        }

        //        // Bullet movement
        //        if (bulletExists) {
        //            bullet_y -= 0.5;
        //            bulletSprite.setPosition(bullet_x, bullet_y);

        //            if (bullet_y < 0) {
        //                bulletExists = false;
        //            }

        //            // Handle collisions with bees
        //            for (int i = 0; i < beeCount; i++) {
        //                if (bees[i].isAlive && bees[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y)) {
        //                    bees[i].isAlive = false;
        //                    deathCount++;

        //                    if (honeycombCount < maxHoneycombs) {
        //                        honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
        //                        if (bees[i].isKiller)
        //                            honeycombs[honeycombCount].sprite.setColor(sf::Color::Red);
        //                        honeycombs[honeycombCount].sprite.setPosition(bees[i].sprite.getPosition());
        //                        honeycombs[honeycombCount].honeycomb_collection.restart();
        //                        honeycombCount++;
        //                    }
        //                    bulletExists = false;
        //                    score += bees[i].isKiller ? 200 : 100;
        //                    cout << "Display Score: " << score << endl;
        //                }
        //            }

        //            // Handle collisions with honeycombs
        //            for (int i = 0; i < honeycombCount; i++) {
        //                if (!honeycombs[i].isCollected &&
        //                    honeycombs[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y) &&
        //                    honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 1.f) {
        //                    honeycombs[i].isCollected = true; // Mark the honeycomb as collected
        //                    bulletExists = false;            // Destroy the bullet
        //                    score += 50;                     // Award points for destroying a honeycomb
        //                    break; // Exit the loop since the bullet can only collide with one honeycomb at a time
        //                }
        //            }
        //        }

        //        // Move bees linearly
        //        if (beeClock.getElapsedTime().asMilliseconds() > 50) {
        //            beeClock.restart();
        //            for (int i = 0; i < beeCount; i++) {
        //                if (bees[i].isAlive) {
        //                    Vector2f pos = bees[i].sprite.getPosition();

        //                    // Worker bee random stop logic
        //                    if (!bees[i].isKiller) { // Only for worker bees
        //                        // Randomly decide to stop (1% chance per frame)
        //                        if (!bees[i].isStopped && rand() % 100 < 1) {
        //                            bees[i].isStopped = true;
        //                            bees[i].stopTimer.restart(); // Start the stop timer
        //                        }

        //                        // Resume movement after stopping for 2 seconds
        //                        if (bees[i].isStopped && bees[i].stopTimer.getElapsedTime().asSeconds() > 2) {
        //                            bees[i].isStopped = false; // Resume movement
        //                        }
        //                    }

        //                    // Handle pollination and exiting
        //                    if (!bees[i].isStopped) { // Only move if the bee is not stopped
        //                        if (pos.y >= resolutionY - (4 * boxPixelsY) && !bees[i].isExiting) {
        //                            bees[i].isExiting = true; // Mark for exit after pollination

        //                            // Determine the side of the screen where the bee is
        //                            if (pos.x > resolutionX / 2) { // Right side
        //                                if (flowerCount < maxFlowers) {
        //                                    if (flowerCountRight == 0) {
        //                                        flowers[flowerCount].sprite.setTexture(flowerTexture);
        //                                        flowers[flowerCount].sprite.setScale(1.f, 1.f);
        //                                        flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
        //                                        flowerCountRight++;
        //                                        flowerCount++;
        //                                    }
        //                                    flowers[flowerCount].sprite.setTexture(flowerTexture);
        //                                    flowers[flowerCount].sprite.setScale(1.f, 1.f);
        //                                    flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
        //                                    flowerCount++;
        //                                    flowerCountRight++;
        //                                    deathCount++;
        //                                }
        //                            }
        //                            else { // Left side
        //                                if (flowerCount < maxFlowers) {
        //                                    if (flowerCountLeft == 0) {
        //                                        flowers[flowerCount].sprite.setTexture(flowerTexture);
        //                                        flowers[flowerCount].sprite.setScale(1.f, 1.f);
        //                                        flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
        //                                        flowerCountLeft++;
        //                                        flowerCount++;
        //                                    }
        //                                    flowers[flowerCount].sprite.setTexture(flowerTexture);
        //                                    flowers[flowerCount].sprite.setScale(1.f, 1.f);
        //                                    flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
        //                                    flowerCount++;
        //                                    flowerCountLeft++;
        //                                    deathCount++;
        //                                }
        //                            }
        //                        }

        //                        // Handle exiting bees
        //                        if (bees[i].isExiting) {
        //                            if (bees[i].sprite.getPosition().x > resolutionX / 2)
        //                                pos.x += bees[i].speed;
        //                            else
        //                                pos.x -= bees[i].speed;
        //                            if (pos.y > resolutionY) {
        //                                bees[i].isAlive = false; // Remove bee once it exits the screen
        //                            }
        //                        }
        //                        else {
        //                            // Normal movement logic
        //                            pos.x += bees[i].beeDirection * bees[i].speed;
        //                            if (pos.x >= resolutionX - boxPixelsX || pos.x <= 0) {
        //                                pos.y += boxPixelsY; // Drop a tier when hitting the screen edge
        //                                bees[i].beeDirection = -bees[i].beeDirection;
        //                            }
        //                        }
        //                    }

        //                    bees[i].sprite.setPosition(pos);
        //                }
        //            }
        //        }

        //        // Eating Honeycombs
        //        for (int i = 0; i < honeycombCount; i++) {
        //            if (!honeycombs[i].isCollected &&
        //                bird.sprite.getGlobalBounds().intersects(honeycombs[i].sprite.getGlobalBounds()) && !honeycombs[i].collection_start) {

        //                honeycombs[i].collection_start = true;
        //                honeycombs[i].honeycomb_collection.restart();

        //                if (honeycombs[i].sprite.getColor() == sf::Color::Red) {
        //                    int row = honeycombs[i].sprite.getPosition().y / rowHeight; // Get the Y-coordinate and divide by the row height

        //                    int points = 1500; // Default points for remaining rows
        //                    if (row == 0 || row == 1) {
        //                        points = 2000; // Top two rows
        //                    }
        //                    else if (row == 2 || row == 3 || row == 4) {
        //                        points = 1800; // Third, fourth, and fifth rows
        //                    }
        //                    score += points;
        //                    cout << "Red Honeycomb" << endl;
        //                }

        //                else {
        //                    int row = honeycombs[i].sprite.getPosition().y / rowHeight; // Get the Y-coordinate and divide by the row height

        //                    int points = 500; // Default points for remaining rows
        //                    if (row == 0 || row == 1) {
        //                        points = 1000; // Top two rows
        //                    }
        //                    else if (row == 2 || row == 3 || row == 4) {
        //                        points = 800; // Third, fourth, and fifth rows

        //                        cout << "Yellow Honeycomb" << endl;
        //                    }
        //                    score += points;
        //                }

        //                cout << "Score: " << score << endl;


        //                // Pause the bird after eating the honeycomb
        //                bird.isPausedAfterHoneycomb = true;
        //                bird.isWaiting = true; // Start waiting phase
        //                bird.waitTimer.restart(); // Reset waiting timer
        //            }
        //        }

        //        // Disappearing Honeycomb
        //        for (int i = 0; i < honeycombCount; i++) {
        //            if (honeycombs[i].collection_start && honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 2.f) {
        //                honeycombs[i].isCollected = true;
        //            }
        //        }

        //        // Sickness
        //        if (bulletExists && bird.sprite.getGlobalBounds().contains(bullet_x, bullet_y)) {
        //            bird.sicknessCount++;
        //            bulletExists = false; // Remove the bullet

        //            if (bird.sicknessCount >= 3) {
        //                bird.isAlive = false;
        //                bird.sicknessCount = 0;
        //                bird.respawnTimer.restart(); // Start respawn timer
        //            }
        //        }

        //        // Render
        //        window.clear();
        //        window.draw(player);

        //        // HUD
        //        displayScoreAndLives(window, score, lives, sprayCount);
        //        if (bulletExists) {
        //            window.draw(bulletSprite);
        //        }

        //        for (int i = 0; i < beeCount; i++) {
        //            if (bees[i].isAlive) window.draw(bees[i].sprite);
        //        }

        //        for (int i = 0; i < honeycombCount; i++) {
        //            if (!honeycombs[i].isCollected) window.draw(honeycombs[i].sprite);
        //        }

        //        for (int i = 0; i < flowerCount; i++) {
        //            window.draw(flowers[i].sprite);
        //        }

        //        // Draw HummingBird
        //        if (bird.isAlive)
        //            window.draw(bird.sprite);

        //        window.display();
        //    }
        //}

        // Game for Level 3
        void level3Game(RenderWindow & window) {
            // Further increase difficulty: even more bees, more honeycombs, etc.
            // You can customize this function as needed for Level 3

            // Load textures
            Texture playerTexture, beeTexture, killerBeeTexture, honeycombTexture, hummingbirdTexture, bulletTexture, flowerTexture;
            if (!playerTexture.loadFromFile("Textures/spray.png") ||
                !flowerTexture.loadFromFile("Textures/obstacles.png") ||
                !beeTexture.loadFromFile("Textures/Regular_bee.png") ||
                !killerBeeTexture.loadFromFile("Textures/Fast_bee.png") ||
                !honeycombTexture.loadFromFile("Textures/honeycomb.png") ||
                !hummingbirdTexture.loadFromFile("Textures/bird.png", sf::IntRect(0, 0, 32, 32)) ||
                !bulletTexture.loadFromFile("Textures/bullet.png")) {
                cout << "Error: Could not load one or more textures!" << endl;
                return;
            }

            // Player setup (reuse global variables)
            Sprite player(playerTexture);
            int flowerRowStart = resolutionY / boxPixelsY - 2;
            player.setPosition(resolutionX / 2 - boxPixelsX / 2, (flowerRowStart - 1) * boxPixelsY);

            // Bees setup with further increased maxBees for difficulty
            maxBees = 30;
            Bee bees[30];
            int beeCount = 0;
            int killerBeeCount = 0;
            int deathCount = 0;
            Clock beeClock, beeGenClock;

            // Flowers setup
            Flower flowers[31];
            int flowerCount = 0; // Start with no flowers
            int flowerCountLeft = 0, flowerCountRight = 0;
            resetFlowers(flowers, flowerCount, flowerRowStart, 0, flowerTexture);

            // HummingBird
            Hummingbird bird;
            bird.birdAppearanceTimer.restart();

            initializeHummingbird(bird, hummingbirdTexture);

            // Bullet setup
            float bullet_x, bullet_y;
            bool bulletExists = false;
            Clock bulletClock;
            Sprite bulletSprite(bulletTexture);
            bulletSprite.setScale(2.5, 2.5);

            Honeycomb honeycombs[45];
            int honeycombCount = 0;

            // Initialize 15 honeycombs at random positions
            for (int i = 0; i < 15; i++) {
                honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
                honeycombs[honeycombCount].sprite.setColor(sf::Color::Red);
                honeycombs[honeycombCount].sprite.setPosition(rand() % (resolutionX - 70), rand() % (resolutionY - 100));
                honeycombs[honeycombCount].honeycomb_collection.restart();
                honeycombCount++;
            }

            while (window.isOpen()) {
                Event e;
                while (window.pollEvent(e)) {
                    if (e.type == Event::Closed) window.close();
                }

                // -------------------
                // Player Movement with Collision Detection
                // -------------------
                // Move Left
                if (Keyboard::isKeyPressed(Keyboard::Left) && player.getPosition().x > 0) {
                    // Calculate the new position
                    Sprite tempPlayer = player;
                    tempPlayer.move(-0.5f, 0);

                    // Check collision with any flower
                    bool collision = false;
                    for (int i = 0; i < flowerCount; i++) {
                        if (tempPlayer.getGlobalBounds().intersects(flowers[i].sprite.getGlobalBounds())) {
                            collision = true;
                            break;
                        }
                    }
                   // if (!collision) {
                        player.move(-0.5f, 0);
                   // }
                }

                // Move Right
                if (Keyboard::isKeyPressed(Keyboard::Right) && player.getPosition().x < resolutionX - boxPixelsX) {
                    // Calculate the new position
                    Sprite tempPlayer = player;
                    tempPlayer.move(0.5f, 0);

                    // Check collision with any flower
                    bool collision = false;
                    for (int i = 0; i < flowerCount; i++) {
                        if (tempPlayer.getGlobalBounds().intersects(flowers[i].sprite.getGlobalBounds())) {
                            collision = true;
                            break;
                        }
                    }
                  //  if (!collision) {
                        player.move(0.5f, 0);
                  //  }
                }

                if (Keyboard::isKeyPressed(Keyboard::X))
                    exit(0);

                // Shooting bullets
                if (Keyboard::isKeyPressed(Keyboard::Space) && !bulletExists && sprayCount > 0) {
                    bullet_x = player.getPosition().x + boxPixelsX / 4;
                    bullet_y = player.getPosition().y;
                    bulletExists = true;
                    sprayCount--;
                }

                // Bee Creation with further increased difficulty
                if (beeGenClock.getElapsedTime().asMilliseconds() > 1000 && beeCount < maxBees) {
                    beeGenClock.restart();
                    int type = rand() % 10;
                    if (type > 7 && killerBeeCount < 15) {
                        initializeBee(bees[beeCount], killerBeeTexture, true, 50, 50);
                        beeCount++;
                        killerBeeCount++;
                    }
                    else {
                        initializeBee(bees[beeCount], beeTexture, false, 50, 50);
                        beeCount++;
                    }
                }

                // HummingBird Behavior
                if (bird.isAlive) {
                    if (bird.isWaiting) {
                        // Wait for 3 seconds before deciding the direction
                        if (bird.moveTimer.getElapsedTime().asSeconds() > 3.f) {
                            bird.isWaiting = false; // End the decision phase
                            // Pick a random direction: -0.5f, 0, or 0.5f for both x and y
                            bird.direction = Direction((rand() % 3 - 1) * 0.5f, (rand() % 3 - 1) * 0.5f);
                            bird.moveTimer.restart(); // Restart the timer for movement
                        }
                    }
                    else {
                        // Move the bird
                        if (bird.moveTimer.getElapsedTime().asSeconds() > 2.f) { // Move for 2 seconds
                            bird.isWaiting = true; // Enter decision phase
                            bird.moveTimer.restart(); // Restart the timer for waiting
                        }

// Convert the position to Direction (for easier manipulation)
Direction pos(bird.sprite.getPosition().x, bird.sprite.getPosition().y);

// Manually add the direction to the position (multiply by scalar)
pos.x += bird.direction.x * 0.15f;
pos.y += bird.direction.y * 0.15f;
                        // Wrap around screen edges
                        if (pos.x < 0) pos.x = resolutionX;
                        if (pos.x > resolutionX) pos.x = 0;
                        if (pos.y < 0) pos.y = resolutionY;
                        if (pos.y > resolutionY) pos.y = 0;

                        bird.sprite.setPosition(pos.x,pos.y );
                    }
                }

                // Hummingbird Appearance
                if (bird.birdAppearanceTimer.getElapsedTime().asSeconds() >= 20.f &&
                    bird.birdAppearanceTimer.getElapsedTime().asSeconds() <= 30.f) {
                    if (!bird.isAlive) {
                        bird.isAlive = true;
                        bird.sprite.setPosition(0, resolutionY);
                    }
                }

                // Lives Management
                if (sprayCount == 0) {
                    if (lives > 0) {
                        lives--;
                        sprayCount = 56;
                    }
                    else {
                        cout << "GameOver" << endl;
                        gameOver(window, score);
                    }
                }
                cout << deathCount << endl;
                if (deathCount == 30)
                    gameOver(window, score);

                // Flower Count Management
                if (flowerCount >= maxFlowers) {
                    cout << "GameOver" << endl;
                    gameOver(window, score);
                }

                // Bee Collision with honeycombs
                for (int i = 0; i < beeCount; i++) {
                    if (bees[i].isAlive) {
                        // Check collision with honeycombs for worker bees
                        if (!bees[i].isKiller) { // Worker bee
                            for (int j = 0; j < honeycombCount; j++) {
                                if (!honeycombs[j].isCollected && bees[i].sprite.getGlobalBounds().intersects(honeycombs[j].sprite.getGlobalBounds())) {
                                    // Worker bee collides with honeycomb
                                    Direction pos ( bees[i].sprite.getPosition().x, bees[i].sprite.getPosition().y);
                                    pos.y += boxPixelsY;
                                    bees[i].sprite.setPosition(pos.x, pos.y);
                                    score += 50;                     // Award points for collision
                                    break;
                                }
                            }
                        }
                    }
                }

                // Bullet movement
                if (bulletExists) {
                    bullet_y -= 0.5;
                    bulletSprite.setPosition(bullet_x, bullet_y);

                    if (bullet_y < 0) {
                        bulletExists = false;
                    }

                    // Handle collisions with bees
                    for (int i = 0; i < beeCount; i++) {
                        if (bees[i].isAlive && bees[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y)) {
                            bees[i].isAlive = false;
                            deathCount++;

                            if (honeycombCount < maxHoneycombs) {
                                honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
                                if (bees[i].isKiller)
                                    honeycombs[honeycombCount].sprite.setColor(sf::Color::Red);
                                honeycombs[honeycombCount].sprite.setPosition(bees[i].sprite.getPosition());
                                honeycombs[honeycombCount].honeycomb_collection.restart();
                                honeycombCount++;
                            }
                            bulletExists = false;
                            score += bees[i].isKiller ? 200 : 100;
                            cout << "Display Score: " << score << endl;
                        }
                    }

                    // Handle collisions with honeycombs
                    for (int i = 0; i < honeycombCount; i++) {
                        if (!honeycombs[i].isCollected &&
                            honeycombs[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y) &&
                            honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 1.f) {
                            honeycombs[i].isCollected = true; // Mark the honeycomb as collected
                            bulletExists = false;            // Destroy the bullet
                            score += 50;                     // Award points for destroying a honeycomb
                            break; // Exit the loop since the bullet can only collide with one honeycomb at a time
                        }
                    }
                }

                // Move bees linearly
                if (beeClock.getElapsedTime().asMilliseconds() > 50) {
                    beeClock.restart();
                    for (int i = 0; i < beeCount; i++) {
                        if (bees[i].isAlive) {
                            Direction pos ( bees[i].sprite.getPosition().x, bees[i].sprite.getPosition().y);

                            // Worker bee random stop logic
                            if (!bees[i].isKiller) { // Only for worker bees
                                // Randomly decide to stop (1% chance per frame)
                                if (!bees[i].isStopped && rand() % 100 < 1) {
                                    bees[i].isStopped = true;
                                    bees[i].stopTimer.restart(); // Start the stop timer
                                }

                                // Resume movement after stopping for 2 seconds
                                if (bees[i].isStopped && bees[i].stopTimer.getElapsedTime().asSeconds() > 2) {
                                    bees[i].isStopped = false; // Resume movement
                                }
                            }

                            // Handle pollination and exiting
                            if (!bees[i].isStopped) { // Only move if the bee is not stopped
                                if (pos.y >= resolutionY - (4 * boxPixelsY) && !bees[i].isExiting) {
                                    bees[i].isExiting = true; // Mark for exit after pollination

                                    // Determine the side of the screen where the bee is
                                    if (pos.x > resolutionX / 2) { // Right side
                                        if (flowerCount < maxFlowers) {
                                            if (flowerCountRight == 0) {
                                                flowers[flowerCount].sprite.setTexture(flowerTexture);
                                                flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                                flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
                                                flowerCountRight++;
                                                flowerCount++;
                                            }
                                            flowers[flowerCount].sprite.setTexture(flowerTexture);
                                            flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                            flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
                                            flowerCount++;
                                            flowerCountRight++;
                                            deathCount++;
                                        }
                                    }
                                    else { // Left side
                                        if (flowerCount < maxFlowers) {
                                            if (flowerCountLeft == 0) {
                                                flowers[flowerCount].sprite.setTexture(flowerTexture);
                                                flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                                flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
                                                flowerCountLeft++;
                                                flowerCount++;
                                            }
                                            flowers[flowerCount].sprite.setTexture(flowerTexture);
                                            flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                            flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
                                            flowerCount++;
                                            flowerCountLeft++;
                                            deathCount++;
                                        }
                                    }
                                }

                                // Handle exiting bees
                                if (bees[i].isExiting) {
                                    if (bees[i].sprite.getPosition().x > resolutionX / 2)
                                        pos.x += bees[i].speed;
                                    else
                                        pos.x -= bees[i].speed;
                                    if (pos.y > resolutionY) {
                                        bees[i].isAlive = false; // Remove bee once it exits the screen
                                    }
                                }
                                else {
                                    // Normal movement logic
                                    pos.x += bees[i].beeDirection * bees[i].speed;
                                    if (pos.x >= resolutionX - boxPixelsX || pos.x <= 0) {
                                        pos.y += boxPixelsY; // Drop a tier when hitting the screen edge
                                        bees[i].beeDirection = -bees[i].beeDirection;
                                    }
                                }
                            }

                            bees[i].sprite.setPosition(pos.x, pos.y);
                        }
                    }
                }

                // Eating Honeycombs
                for (int i = 0; i < honeycombCount; i++) {
                    if (!honeycombs[i].isCollected &&
                        bird.sprite.getGlobalBounds().intersects(honeycombs[i].sprite.getGlobalBounds()) && !honeycombs[i].collection_start) {

                        honeycombs[i].collection_start = true;
                        honeycombs[i].honeycomb_collection.restart();

                        if (honeycombs[i].sprite.getColor() == sf::Color::Red) {
                            int row = honeycombs[i].sprite.getPosition().y / rowHeight; // Get the Y-coordinate and divide by the row height

                            int points = 1500; // Default points for remaining rows
                            if (row == 0 || row == 1) {
                                points = 2000; // Top two rows
                            }
                            else if (row == 2 || row == 3 || row == 4) {
                                points = 1800; // Third, fourth, and fifth rows
                            }
                            score += points;
                            cout << "Red Honeycomb" << endl;
                        }

                        else {
                            int row = honeycombs[i].sprite.getPosition().y / rowHeight; // Get the Y-coordinate and divide by the row height

                            int points = 500; // Default points for remaining rows
                            if (row == 0 || row == 1) {
                                points = 1000; // Top two rows
                            }
                            else if (row == 2 || row == 3 || row == 4) {
                                points = 800; // Third, fourth, and fifth rows

                                cout << "Yellow Honeycomb" << endl;
                            }
                            score += points;
                        }

                        cout << "Score: " << score << endl;


                        // Pause the bird after eating the honeycomb
                        bird.isPausedAfterHoneycomb = true;
                        bird.isWaiting = true; // Start waiting phase
                        bird.waitTimer.restart(); // Reset waiting timer
                    }
                }

                // Disappearing Honeycomb
                for (int i = 0; i < honeycombCount; i++) {
                    if (honeycombs[i].collection_start && honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 2.f) {
                        honeycombs[i].isCollected = true;
                    }
                }

                // Sickness
                if (bulletExists && bird.sprite.getGlobalBounds().contains(bullet_x, bullet_y)) {
                    bird.sicknessCount++;
                    bulletExists = false; // Remove the bullet

                    if (bird.sicknessCount >= 3) {
                        bird.isAlive = false;
                        bird.sicknessCount = 0;
                        bird.respawnTimer.restart(); // Start respawn timer
                    }
                }

                // Render
                window.clear();
                window.draw(player);

                // HUD
                displayScoreAndLives(window, score, lives, sprayCount);
                if (bulletExists) {
                    window.draw(bulletSprite);
                }

                for (int i = 0; i < beeCount; i++) {
                    if (bees[i].isAlive) window.draw(bees[i].sprite);
                }

                for (int i = 0; i < honeycombCount; i++) {
                    if (!honeycombs[i].isCollected) window.draw(honeycombs[i].sprite);
                }

                for (int i = 0; i < flowerCount; i++) {
                    window.draw(flowers[i].sprite);
                }

                // Draw HummingBird
                if (bird.isAlive)
                    window.draw(bird.sprite);

                window.display();
            }
        }

        // Game for Level 3
        //void level3Game(RenderWindow & window) {
        //    // Further increase difficulty: even more bees, more honeycombs, etc.
        //    // You can customize this function as needed for Level 3

        //    // Load textures
        //    Texture playerTexture, beeTexture, killerBeeTexture, honeycombTexture, hummingbirdTexture, bulletTexture, flowerTexture;
        //    if (!playerTexture.loadFromFile("Textures/spray.png") ||
        //        !flowerTexture.loadFromFile("Textures/obstacles.png") ||
        //        !beeTexture.loadFromFile("Textures/Regular_bee.png") ||
        //        !killerBeeTexture.loadFromFile("Textures/Fast_bee.png") ||
        //        !honeycombTexture.loadFromFile("Textures/honeycomb.png") ||
        //        !hummingbirdTexture.loadFromFile("Textures/bird.png", sf::IntRect(0, 0, 32, 32)) ||
        //        !bulletTexture.loadFromFile("Textures/bullet.png")) {
        //        cout << "Error: Could not load one or more textures!" << endl;
        //        return;
        //    }

        //    // Player setup (reuse global variables)
        //    Sprite player(playerTexture);
        //    int flowerRowStart = resolutionY / boxPixelsY - 2;
        //    player.setPosition(resolutionX / 2 - boxPixelsX / 2, (flowerRowStart - 1) * boxPixelsY);

        //    // Bees setup with further increased maxBees for difficulty
        //    Bee bees[30];
        //    int beeCount = 0;
        //    int killerBeeCount = 0;
        //    int deathCount = 0;
        //    Clock beeClock, beeGenClock;

        //    // Flowers setup
        //    Flower flowers[31];
        //    int flowerCount = 0; // Start with no flowers
        //    int flowerCountLeft = 0, flowerCountRight = 0;
        //    resetFlowers(flowers, flowerCount, flowerRowStart, 0, flowerTexture);

        //    // HummingBird
        //    Hummingbird bird;
        //    bird.birdAppearanceTimer.restart();

        //    initializeHummingbird(bird, hummingbirdTexture);

        //    // Bullet setup
        //    float bullet_x, bullet_y;
        //    bool bulletExists = false;
        //    Clock bulletClock;
        //    Sprite bulletSprite(bulletTexture);
        //    bulletSprite.setScale(2.5, 2.5);

        //    Honeycomb honeycombs[45];
        //    int honeycombCount = 0;

        //    // Initialize 15 honeycombs at random positions
        //    for (int i = 0; i < 15; i++) {
        //        honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
        //        honeycombs[honeycombCount].sprite.setColor(sf::Color::Red);
        //        honeycombs[honeycombCount].sprite.setPosition(rand() % (resolutionX - 70), rand() % (resolutionY - 100));
        //        honeycombs[honeycombCount].honeycomb_collection.restart();
        //        honeycombCount++;
        //    }

        //    while (window.isOpen()) {
        //        Event e;
        //        while (window.pollEvent(e)) {
        //            if (e.type == Event::Closed) window.close();
        //        }

        //        // -------------------
        //        // Player Movement with Collision Detection
        //        // -------------------
        //        // Move Left
        //        if (Keyboard::isKeyPressed(Keyboard::Left) && player.getPosition().x > 0) {
        //            // Calculate the new position
        //            Sprite tempPlayer = player;
        //            tempPlayer.move(-0.5f, 0);

        //            // Check collision with any flower
        //            bool collision = false;
        //            for (int i = 0; i < flowerCount; i++) {
        //                if (tempPlayer.getGlobalBounds().intersects(flowers[i].sprite.getGlobalBounds())) {
        //                    collision = true;
        //                    break;
        //                }
        //            }
        //            if (!collision) {
        //                player.move(-0.5f, 0);
        //            }
        //        }

        //        // Move Right
        //        if (Keyboard::isKeyPressed(Keyboard::Right) && player.getPosition().x < resolutionX - boxPixelsX) {
        //            // Calculate the new position
        //            Sprite tempPlayer = player;
        //            tempPlayer.move(0.5f, 0);

        //            // Check collision with any flower
        //            bool collision = false;
        //            for (int i = 0; i < flowerCount; i++) {
        //                if (tempPlayer.getGlobalBounds().intersects(flowers[i].sprite.getGlobalBounds())) {
        //                    collision = true;
        //                    break;
        //                }
        //            }
        //            if (!collision) {
        //                player.move(0.5f, 0);
        //            }
        //        }

        //        if (Keyboard::isKeyPressed(Keyboard::X))
        //            exit(0);

        //        // Shooting bullets
        //        if (Keyboard::isKeyPressed(Keyboard::Space) && !bulletExists && sprayCount > 0) {
        //            bullet_x = player.getPosition().x + boxPixelsX / 4;
        //            bullet_y = player.getPosition().y;
        //            bulletExists = true;
        //            sprayCount--;
        //        }

        //        // Bee Creation with further increased difficulty
        //        if (beeGenClock.getElapsedTime().asMilliseconds() > 1000 && beeCount < maxBees) {
        //            beeGenClock.restart();
        //            int type = rand() % 10;
        //            if (type > 7 && killerBeeCount < 15) {
        //                initializeBee(bees[beeCount], killerBeeTexture, true, 50, 50);
        //                beeCount++;
        //                killerBeeCount++;
        //            }
        //            else {
        //                initializeBee(bees[beeCount], beeTexture, false, 50, 50);
        //                beeCount++;
        //            }
        //        }

        //        // HummingBird Behavior
        //        if (bird.isAlive) {
        //            if (bird.isWaiting) {
        //                // Wait for 3 seconds before deciding the direction
        //                if (bird.moveTimer.getElapsedTime().asSeconds() > 3.f) {
        //                    bird.isWaiting = false; // End the decision phase
        //                    // Pick a random direction: -0.5f, 0, or 0.5f for both x and y
        //                    bird.direction = Vector2f((rand() % 3 - 1) * 0.5f, (rand() % 3 - 1) * 0.5f);
        //                    bird.moveTimer.restart(); // Restart the timer for movement
        //                }
        //            }
        //            else {
        //                // Move the bird
        //                if (bird.moveTimer.getElapsedTime().asSeconds() > 2.f) { // Move for 2 seconds
        //                    bird.isWaiting = true; // Enter decision phase
        //                    bird.moveTimer.restart(); // Restart the timer for waiting
        //                }

        //                Vector2f pos = bird.sprite.getPosition();
        //                pos += bird.direction * 0.15f; // Adjust movement speed if needed

        //                // Wrap around screen edges
        //                if (pos.x < 0) pos.x = resolutionX;
        //                if (pos.x > resolutionX) pos.x = 0;
        //                if (pos.y < 0) pos.y = resolutionY;
        //                if (pos.y > resolutionY) pos.y = 0;

        //                bird.sprite.setPosition(pos);
        //            }
        //        }

        //        // Hummingbird Appearance
        //        if (bird.birdAppearanceTimer.getElapsedTime().asSeconds() >= 20.f &&
        //            bird.birdAppearanceTimer.getElapsedTime().asSeconds() <= 30.f) {
        //            if (!bird.isAlive) {
        //                bird.isAlive = true;
        //                bird.sprite.setPosition(0, resolutionY);
        //            }
        //        }

        //        // Lives Management
        //        if (sprayCount == 0) {
        //            if (lives > 0) {
        //                lives--;
        //                sprayCount = 56;
        //            }
        //            else {
        //                cout << "GameOver" << endl;
        //                gameOver(window, score);
        //            }
        //        }

        //        // Flower Count Management
        //        if (flowerCount >= maxFlowers) {
        //            cout << "GameOver" << endl;
        //            gameOver(window, score);
        //        }

        //        // Bee Collision with honeycombs
        //        for (int i = 0; i < beeCount; i++) {
        //            if (bees[i].isAlive) {
        //                // Check collision with honeycombs for worker bees
        //                if (!bees[i].isKiller) { // Worker bee
        //                    for (int j = 0; j < honeycombCount; j++) {
        //                        if (!honeycombs[j].isCollected && bees[i].sprite.getGlobalBounds().intersects(honeycombs[j].sprite.getGlobalBounds())) {
        //                            // Worker bee collides with honeycomb
        //                            Vector2f pos = bees[i].sprite.getPosition();
        //                            pos.y += boxPixelsY;
        //                            bees[i].sprite.setPosition(pos);
        //                            score += 50;                     // Award points for collision
        //                            break;
        //                        }
        //                    }
        //                }
        //            }
        //        }

        //        // Bullet movement
        //        if (bulletExists) {
        //            bullet_y -= 0.5;
        //            bulletSprite.setPosition(bullet_x, bullet_y);

        //            if (bullet_y < 0) {
        //                bulletExists = false;
        //            }

        //            // Handle collisions with bees
        //            for (int i = 0; i < beeCount; i++) {
        //                if (bees[i].isAlive && bees[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y)) {
        //                    bees[i].isAlive = false;
        //                    deathCount++;

        //                    if (honeycombCount < maxHoneycombs) {
        //                        honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
        //                        if (bees[i].isKiller)
        //                            honeycombs[honeycombCount].sprite.setColor(sf::Color::Red);
        //                        honeycombs[honeycombCount].sprite.setPosition(bees[i].sprite.getPosition());
        //                        honeycombs[honeycombCount].honeycomb_collection.restart();
        //                        honeycombCount++;
        //                    }
        //                    bulletExists = false;
        //                    score += bees[i].isKiller ? 200 : 100;
        //                    cout << "Display Score: " << score << endl;
        //                }
        //            }

        //            // Handle collisions with honeycombs
        //            for (int i = 0; i < honeycombCount; i++) {
        //                if (!honeycombs[i].isCollected &&
        //                    honeycombs[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y) &&
        //                    honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 1.f) {
        //                    honeycombs[i].isCollected = true; // Mark the honeycomb as collected
        //                    bulletExists = false;            // Destroy the bullet
        //                    score += 50;                     // Award points for destroying a honeycomb
        //                    break; // Exit the loop since the bullet can only collide with one honeycomb at a time
        //                }
        //            }
        //        }

        //        // Move bees linearly
        //        if (beeClock.getElapsedTime().asMilliseconds() > 50) {
        //            beeClock.restart();
        //            for (int i = 0; i < beeCount; i++) {
        //                if (bees[i].isAlive) {
        //                    Vector2f pos = bees[i].sprite.getPosition();

        //                    // Worker bee random stop logic
        //                    if (!bees[i].isKiller) { // Only for worker bees
        //                        // Randomly decide to stop (1% chance per frame)
        //                        if (!bees[i].isStopped && rand() % 100 < 1) {
        //                            bees[i].isStopped = true;
        //                            bees[i].stopTimer.restart(); // Start the stop timer
        //                        }

        //                        // Resume movement after stopping for 2 seconds
        //                        if (bees[i].isStopped && bees[i].stopTimer.getElapsedTime().asSeconds() > 2) {
        //                            bees[i].isStopped = false; // Resume movement
        //                        }
        //                    }

        //                    // Handle pollination and exiting
        //                    if (!bees[i].isStopped) { // Only move if the bee is not stopped
        //                        if (pos.y >= resolutionY - (4 * boxPixelsY) && !bees[i].isExiting) {
        //                            bees[i].isExiting = true; // Mark for exit after pollination

        //                            // Determine the side of the screen where the bee is
        //                            if (pos.x > resolutionX / 2) { // Right side
        //                                if (flowerCount < maxFlowers) {
        //                                    if (flowerCountRight == 0) {
        //                                        flowers[flowerCount].sprite.setTexture(flowerTexture);
        //                                        flowers[flowerCount].sprite.setScale(1.f, 1.f);
        //                                        flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
        //                                        flowerCountRight++;
        //                                        flowerCount++;
        //                                    }
        //                                    flowers[flowerCount].sprite.setTexture(flowerTexture);
        //                                    flowers[flowerCount].sprite.setScale(1.f, 1.f);
        //                                    flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
        //                                    flowerCount++;
        //                                    flowerCountRight++;
        //                                    deathCount++;
        //                                }
        //                            }
        //                            else { // Left side
        //                                if (flowerCount < maxFlowers) {
        //                                    if (flowerCountLeft == 0) {
        //                                        flowers[flowerCount].sprite.setTexture(flowerTexture);
        //                                        flowers[flowerCount].sprite.setScale(1.f, 1.f);
        //                                        flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
        //                                        flowerCountLeft++;
        //                                        flowerCount++;
        //                                    }
        //                                    flowers[flowerCount].sprite.setTexture(flowerTexture);
        //                                    flowers[flowerCount].sprite.setScale(1.f, 1.f);
        //                                    flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
        //                                    flowerCount++;
        //                                    flowerCountLeft++;
        //                                    deathCount++;
        //                                }
        //                            }
        //                        }

        //                        // Handle exiting bees
        //                        if (bees[i].isExiting) {
        //                            if (bees[i].sprite.getPosition().x > resolutionX / 2)
        //                                pos.x += bees[i].speed;
        //                            else
        //                                pos.x -= bees[i].speed;
        //                            if (pos.y > resolutionY) {
        //                                bees[i].isAlive = false; // Remove bee once it exits the screen
        //                            }
        //                        }
        //                        else {
        //                            // Normal movement logic
        //                            pos.x += bees[i].beeDirection * bees[i].speed;
        //                            if (pos.x >= resolutionX - boxPixelsX || pos.x <= 0) {
        //                                pos.y += boxPixelsY; // Drop a tier when hitting the screen edge
        //                                bees[i].beeDirection = -bees[i].beeDirection;
        //                            }
        //                        }
        //                    }

        //                    bees[i].sprite.setPosition(pos);
        //                }
        //            }
        //        }

        //        // Eating Honeycombs
        //        for (int i = 0; i < honeycombCount; i++) {
        //            if (!honeycombs[i].isCollected &&
        //                bird.sprite.getGlobalBounds().intersects(honeycombs[i].sprite.getGlobalBounds()) && !honeycombs[i].collection_start) {

        //                honeycombs[i].collection_start = true;
        //                honeycombs[i].honeycomb_collection.restart();

        //                if (honeycombs[i].sprite.getColor() == sf::Color::Red) {
        //                    int row = honeycombs[i].sprite.getPosition().y / rowHeight; // Get the Y-coordinate and divide by the row height

        //                    int points = 1500; // Default points for remaining rows
        //                    if (row == 0 || row == 1) {
        //                        points = 2000; // Top two rows
        //                    }
        //                    else if (row == 2 || row == 3 || row == 4) {
        //                        points = 1800; // Third, fourth, and fifth rows
        //                    }
        //                    score += points;
        //                    cout << "Red Honeycomb" << endl;
        //                }

        //                else {
        //                    int row = honeycombs[i].sprite.getPosition().y / rowHeight; // Get the Y-coordinate and divide by the row height

        //                    int points = 500; // Default points for remaining rows
        //                    if (row == 0 || row == 1) {
        //                        points = 1000; // Top two rows
        //                    }
        //                    else if (row == 2 || row == 3 || row == 4) {
        //                        points = 800; // Third, fourth, and fifth rows

        //                        cout << "Yellow Honeycomb" << endl;
        //                    }
        //                    score += points;
        //                }

        //                cout << "Score: " << score << endl;


        //                // Pause the bird after eating the honeycomb
        //                bird.isPausedAfterHoneycomb = true;
        //                bird.isWaiting = true; // Start waiting phase
        //                bird.waitTimer.restart(); // Reset waiting timer
        //            }
        //        }

        //        // Disappearing Honeycomb
        //        for (int i = 0; i < honeycombCount; i++) {
        //            if (honeycombs[i].collection_start && honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 2.f) {
        //                honeycombs[i].isCollected = true;
        //            }
        //        }

        //        // Sickness
        //        if (bulletExists && bird.sprite.getGlobalBounds().contains(bullet_x, bullet_y)) {
        //            bird.sicknessCount++;
        //            bulletExists = false; // Remove the bullet

        //            if (bird.sicknessCount >= 3) {
        //                bird.isAlive = false;
        //                bird.sicknessCount = 0;
        //                bird.respawnTimer.restart(); // Start respawn timer
        //            }
        //        }

        //        // Render
        //        window.clear();
        //        window.draw(player);

        //        // HUD
        //        displayScoreAndLives(window, score, lives, sprayCount);
        //        if (bulletExists) {
        //            window.draw(bulletSprite);
        //        }

        //        for (int i = 0; i < beeCount; i++) {
        //            if (bees[i].isAlive) window.draw(bees[i].sprite);
        //        }

        //        for (int i = 0; i < honeycombCount; i++) {
        //            if (!honeycombs[i].isCollected) window.draw(honeycombs[i].sprite);
        //        }

        //        for (int i = 0; i < flowerCount; i++) {
        //            window.draw(flowers[i].sprite);
        //        }

        //        // Draw HummingBird
        //        if (bird.isAlive)
        //            window.draw(bird.sprite);

        //        window.display();
        //    }
        //}

void level4Game(RenderWindow& window) {
    // Load textures
    Texture playerTexture, beeTexture, killerBeeTexture, honeycombTexture, hummingbirdTexture, bulletTexture, flowerTexture;
    Texture powerUpTexture, infantBeeTexture, hunterBeeTexture, hiveTexture;

    if (!playerTexture.loadFromFile("Textures/spray.png") ||
        !flowerTexture.loadFromFile("Textures/obstacles.png") ||
        !beeTexture.loadFromFile("Textures/Regular_bee.png") ||
        !killerBeeTexture.loadFromFile("Textures/Fast_bee.png") ||
        !honeycombTexture.loadFromFile("Textures/honeycomb.png") ||
        !hummingbirdTexture.loadFromFile("Textures/bird.png", IntRect(0, 0, 32, 32)) ||
        !bulletTexture.loadFromFile("Textures/bullet.png") ||
        !powerUpTexture.loadFromFile("Textures/hive.png") || // Ensure you have a power-up texture
        !infantBeeTexture.loadFromFile("Textures/Regular_bee.png") || // Ensure you have an infant bee texture
        !hunterBeeTexture.loadFromFile("Textures/Fast_bee.png") || // Ensure you have a hunter bee texture
        !hiveTexture.loadFromFile("Textures/hive.png")) { // Ensure you have a hive texture
        cout << "Error: Could not load one or more textures!" << endl;
        return;
    }

    // Player setup
    Sprite player(playerTexture);
    int flowerRowStart = resolutionY / boxPixelsY - 2;
    player.setPosition(resolutionX / 2 - boxPixelsX / 2, (flowerRowStart - 1) * boxPixelsY);

    // Bees setup
    const int totalRegularBees = 20;
    const int totalFastBees = 15;
    Bee bees[35]; // 20 regular + 15 fast
    int beeCount = 0;
    int killerBeeCount = 0;
    int deathCount = 0;
    Clock beeClock, beeGenClock;

    // Initialize regular bees
    for (int i = 0; i < totalRegularBees; i++) {
        initializeBee(bees[beeCount], beeTexture, false, rand() % (resolutionX - boxPixelsX), rand() % (resolutionY / 2));
        beeCount++;
    }

    // Initialize fast bees
    for (int i = 0; i < totalFastBees; i++) {
        initializeBee(bees[beeCount], killerBeeTexture, true, rand() % (resolutionX - boxPixelsX), rand() % (resolutionY / 2));
        beeCount++;
    }

    // Flowers setup
    Flower flowers[31];
    int flowerCount = 5; // Starting with 5 flowers
    int flowerCountLeft = 0, flowerCountRight = 0;
    resetFlowers(flowers, flowerCount, flowerRowStart, 0, flowerTexture);

    // Hives setup
    const int totalHives = 5;
    Sprite hives[5];
    for (int i = 0; i < totalHives; i++) {
        hives[i].setTexture(hiveTexture);
        hives[i].setPosition(rand() % (resolutionX - 70), rand() % (resolutionY / 2));
    }

    // Honeycombs setup
    Honeycomb honeycombs[15];
    int honeycombCount = 0;
    for (int i = 0; i < 15; i++) {
        honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
        honeycombs[honeycombCount].sprite.setColor(Color::Red);
        honeycombs[honeycombCount].sprite.setPosition(rand() % (resolutionX - 70), rand() % (resolutionY - 100));
        honeycombs[honeycombCount].honeycomb_collection.restart();
        honeycombCount++;
    }

    // Power-Ups setup
    Texture powerUpIcons[4];
    // Load individual icons for each power-up type if different
    // Alternatively, use the same texture and differentiate by type
    // Here, assuming same texture with different colors
    powerUpIcons[0].loadFromFile("Textures/powerup_speed_increase.png");
    powerUpIcons[1].loadFromFile("Textures/powerup_speed_decrease.png");
    powerUpIcons[2].loadFromFile("Textures/powerup_height_increase.png");
    powerUpIcons[3].loadFromFile("Textures/powerup_height_decrease.png");
    // Handle loading errors as needed

    // Infant Bee setup
    
    Clock infantBeeSpawnClock;
    const float infantBeeSpawnInterval = 30.f; // Adjust as needed

    // Bullet setup
    float bullet_x, bullet_y;
    bool bulletExists = false;
    Clock bulletClock;
    Sprite bulletSprite(bulletTexture);
    bulletSprite.setScale(2.5f, 2.5f);

    // Hummingbird setup
    Hummingbird bird;
    bird.birdAppearanceTimer.restart();
    initializeHummingbird(bird, hummingbirdTexture);

    // Variables to manage power-up effects
    bool speedIncreased = false;
    bool speedDecreased = false;
    bool heightIncreased = false;
    bool heightDecreased = false;

    // Main game loop
    while (window.isOpen()) {
        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed)
                window.close();
        }

        // Player Movement with Collision Detection
        if (Keyboard::isKeyPressed(Keyboard::Left) && player.getPosition().x > 0) {
            Sprite tempPlayer = player;
            tempPlayer.move(-0.5f, 0);
            //bool collision = checkPlayerCollisionWithFlowers(tempPlayer, flowers, flowerCount);
            //if (!collision) {
                player.move(-0.5f, 0);
           // }
        }

        if (Keyboard::isKeyPressed(Keyboard::Right) && player.getPosition().x < resolutionX - boxPixelsX) {
            Sprite tempPlayer = player;
            tempPlayer.move(0.5f, 0);
           // bool collision = checkPlayerCollisionWithFlowers(tempPlayer, flowers, flowerCount);
            //if (!collision) {
                player.move(0.5f, 0);
            //}
        }

        if (Keyboard::isKeyPressed(Keyboard::X))
            exit(0);

        // Shooting bullets
        if (Keyboard::isKeyPressed(Keyboard::Space) && !bulletExists && sprayCount > 0) {
            bullet_x = player.getPosition().x + boxPixelsX / 4;
            bullet_y = player.getPosition().y;
            bulletExists = true;
            sprayCount--;
        }

        // Bee Generation
        if (beeGenClock.getElapsedTime().asMilliseconds() > 1000 && beeCount < 35) { // Adjust interval as needed
            beeGenClock.restart();
            // Bees are already initialized; no need to spawn dynamically
            // If dynamic spawning is needed, implement here
        }

        // HummingBird Behavior
        if (bird.isAlive) {
            if (bird.isWaiting) {
                if (bird.moveTimer.getElapsedTime().asSeconds() > 3.f) {
                    bird.isWaiting = false;
                    bird.direction = Direction((rand() % 3 - 1) * 0.5f, (rand() % 3 - 1) * 0.5f);
                    bird.moveTimer.restart();
                }
            }
            else {
                if (bird.moveTimer.getElapsedTime().asSeconds() > 2.f) {
                    bird.isWaiting = true;
                    bird.moveTimer.restart();
                }

// Convert the position to Direction (for easier manipulation)
Direction pos(bird.sprite.getPosition().x, bird.sprite.getPosition().y);

// Manually add the direction to the position (multiply by scalar)
pos.x += bird.direction.x * 0.15f;
pos.y += bird.direction.y * 0.15f;
                if (pos.x < 0) pos.x = resolutionX;
                if (pos.x > resolutionX) pos.x = 0;
                if (pos.y < 0) pos.y = resolutionY;
                if (pos.y > resolutionY) pos.y = 0;

                bird.sprite.setPosition(pos.x, pos.y);
            }
        }

        // Hummingbird Appearance
        if (bird.birdAppearanceTimer.getElapsedTime().asSeconds() >= 20.f &&
            bird.birdAppearanceTimer.getElapsedTime().asSeconds() <= 30.f) {
            if (!bird.isAlive) {
                bird.isAlive = true;
                bird.sprite.setPosition(0, resolutionY);
            }
        }

        // Power-Up Spawning Conditions
        // Spawn power-up when hummingbird eats a honeycomb
        for (int i = 0; i < honeycombCount; i++) {
    if (honeycombs[i].collection_start && honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 1.f) {
        // Randomly decide to spawn a power-up
        if (rand() % 2 == 0) { // 50% chance
            if (nextPowerUpIndex < MAX_POWER_UPS) { // Check if there is space in the array
                PowerUp newPowerUp;
                int powerType = (rand() % 4) + 1; // 1 to 4
                initializePowerUp(newPowerUp, powerUpIcons[powerType - 1], powerType, honeycombs[i].sprite.getPosition().x, honeycombs[i].sprite.getPosition().y);

                // Place the new power-up in the array and increment the counter
                powerUps[nextPowerUpIndex] = newPowerUp;
                nextPowerUpIndex++;  // Move to the next available slot
            }
        }
        // Reset collection flag
        honeycombs[i].isCollected = true;
    }
}


        // Spawn power-up when spray can shoots red honeycombs hitting hunter bees
        // Assuming that shooting a red honeycomb is handled similarly to hitting a hunter bee
        // Implement logic here if applicable

        // Power-Up Pickup and Effects
        for (int i = 0; i < MAX_POWER_UPS; ++i) {
    if (!powerUps[i].isActive) continue; // Skip inactive power-ups

    // Check if power-up is still on the ground
    if (powerUps[i].timer.getElapsedTime().asSeconds() > 10.f) { // Power-up disappears after 10 seconds
        powerUps[i].isActive = false; // Mark as inactive
        continue;
    }

    // Check collision with player
    if (player.getGlobalBounds().intersects(powerUps[i].sprite.getGlobalBounds())) {
        // Apply power-up effect
        switch (powerUps[i].type) {
            case 1: // Speed Increase
                speedIncreased = true;
                player.move(0.f, 0.f); // Example change; adjust as needed
                break;
            case 2: // Speed Decrease
                speedDecreased = true;
                player.move(0.f, 0.f); // Example change; adjust as needed
                break;
            case 3: // Height Increase
                heightIncreased = true;
                // Modify spray can height; implement accordingly
                break;
            case 4: // Height Decrease
                heightDecreased = true;
                // Modify spray can height; implement accordingly
                break;
        }
        // Start or reset the timer
        powerUps[i].timer.restart();
        // Mark the power-up as inactive
        powerUps[i].isActive = false;
        continue;
    }
}


        // Manage Power-Up Effects Duration
        // You can visualize the timer bars here
        // Example: Check each active power-up and update accordingly
        // Implement visual timer bars as separate UI elements if needed

        // Infant Bee Spawning
       if (infantBeeSpawnClock.getElapsedTime().asSeconds() > infantBeeSpawnInterval) {
    infantBeeSpawnClock.restart();

    // Attempt to spawn an Infant Bee from a random hive
    int spawnHive = rand() % totalHives;
    Sprite& selectedHive = hives[spawnHive];
    Direction spawnPos(selectedHive.getPosition().x, selectedHive.getPosition().y);
    spawnPos.y -= selectedHive.getGlobalBounds().height; // Top of the hive

    // Check if spawn position is obstructed
    bool obstructed = false;
    for (int i = 0; i < honeycombCount; i++) {
        if (honeycombs[i].sprite.getGlobalBounds().contains(spawnPos.x, spawnPos.y)) {
            obstructed = true;
            break;
        }
    }

    // If the spawn position is not obstructed, spawn the bee
    if (!obstructed && nextInfantBeeIndex < MAX_INFANT_BEES) {
        InfantBee newInfantBee;
        initializeInfantBee(newInfantBee, infantBeeTexture, spawnPos.x, spawnPos.y);

        // Place the new InfantBee in the array
        infantBees[nextInfantBeeIndex] = newInfantBee;
        nextInfantBeeIndex++;  // Increment to track the next available slot
    }
}


        // Move Infant Bees
     for (int i = 0; i < nextInfantBeeIndex; ++i) {
    if (!infantBees[i].isAlive) {
        continue;  // Skip inactive bees
    }

    if (!infantBees[i].isTransformed) {
        // Attempt to move upward
        Direction pos(infantBees[i].sprite.getPosition().x, infantBees[i].sprite.getPosition().y);
        pos.y -= 1.f; // Adjust speed as needed

        // Check for obstacles
        bool obstacle = false;
        for (int j = 0; j < honeycombCount; j++) {
            if (honeycombs[j].sprite.getGlobalBounds().intersects(infantBees[i].sprite.getGlobalBounds())) {
                obstacle = true;
                break;
            }
        }

        if (!obstacle) {
            infantBees[i].sprite.setPosition(pos.x, pos.y);
        } else {
            // Try moving left or right
            bool moved = false;
            Direction leftPos(pos);
            leftPos.x -= 1.f;

            // Check if left move is possible
            bool leftBlocked = false;
            for (int j = 0; j < honeycombCount; j++) {
                if (honeycombs[j].sprite.getGlobalBounds().contains(leftPos.x, leftPos.y)) {
                    leftBlocked = true;
                    break;
                }
            }

            if (!leftBlocked && leftPos.x > 0) {
                pos.x -= 1.f;
                infantBees[i].sprite.setPosition(pos.x, pos.y);
                moved = true;
            }

            Direction rightPos = pos;
            rightPos.x += 1.f;
            bool rightBlocked = false;
            for (int j = 0; j < honeycombCount; j++) {
                if (honeycombs[j].sprite.getGlobalBounds().contains(rightPos.x, rightPos.y)) {
                    rightBlocked = true;
                    break;
                }
            }

            if (!rightBlocked && rightPos.x < resolutionX - boxPixelsX) {
                pos.x += 1.f;
                infantBees[i].sprite.setPosition(pos.x, pos.y);
                moved = true;
            }

            if (!moved) {
                // Trapped: transform into a new hive
                Sprite newHive = hives[0]; // Reuse first hive sprite; adjust as needed
                newHive.setPosition(infantBees[i].sprite.getPosition());
                hives[0].setPosition(rand() % (resolutionX - 70), rand() % (resolutionY / 2));
                infantBees[i].isAlive = false;  // Mark the bee as inactive
                continue;  // Skip the rest of the loop
            }
        }

        // Check if reached top to transform into Hunter Bee
        if (infantBees[i].sprite.getPosition().y <= 0) {
            infantBees[i].isTransformed = true;
            infantBees[i].sprite.setTexture(hunterBeeTexture);
            infantBees[i].direction = Direction(0.f, 1.f); // Start moving downward
        }
    } else {
        // Hunter Bee behavior: fly downward
        Direction pos(infantBees[i].sprite.getPosition().x, infantBees[i].sprite.getPosition().y);
        pos.y += 2.f; // Adjust speed as needed
        infantBees[i].sprite.setPosition(pos.x, pos.y);

        // Check collision with player
        if (player.getGlobalBounds().intersects(infantBees[i].sprite.getGlobalBounds())) {
            // Handle collision (e.g., reduce lives or end game)
            // Implement as per game mechanics
        }

        // If reaches bottom, mark as inactive
        if (infantBees[i].sprite.getPosition().y > resolutionY) {
            infantBees[i].isAlive = false;
            continue;  // Skip the rest of the loop for this bee
        }
    }
}

        // Lives Management
        if (sprayCount == 0) {
            if (lives > 0) {
                lives--;
                sprayCount = 56;
            }
            else {
                cout << "GameOver" << endl;
                gameOver(window, score);
            }
        }

        if (deathCount >= 50) { // Adjust game over condition as needed
            cout << "GameOver" << endl;
            gameOver(window, score);
        }

        // Flower Count Management
        if (flowerCount >= maxFlowers) {
            cout << "GameOver" << endl;
            gameOver(window, score);
        }

        // Bee Collision with honeycombs
        for (int i = 0; i < beeCount; i++) {
            if (bees[i].isAlive) {
                if (!bees[i].isKiller) { // Worker bee
                    for (int j = 0; j < honeycombCount; j++) {
                        if (!honeycombs[j].isCollected && bees[i].sprite.getGlobalBounds().intersects(honeycombs[j].sprite.getGlobalBounds())) {
                            Direction pos = (bees[i].sprite.getPosition().x, bees[i].sprite.getPosition().y);
                            pos.y += boxPixelsY;
                            bees[i].sprite.setPosition(pos.x, pos.y);
                            score += 50;
                            break;
                        }
                    }
                }
            }
        }

        // Bullet movement
        if (bulletExists) {
            bullet_y -= 0.5f;
            bulletSprite.setPosition(bullet_x, bullet_y);

            if (bullet_y < 0) {
                bulletExists = false;
            }

            // Handle collisions with bees
            for (int i = 0; i < beeCount; i++) {
                if (bees[i].isAlive && bees[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y)) {
                    // Check if it's an Infant Bee in child form
                    bool isInfantBee = false;
                    for (auto& infantBee : infantBees) {
                        if (infantBee.isAlive && infantBee.sprite.getGlobalBounds().intersects(bees[i].sprite.getGlobalBounds())) {
                            isInfantBee = true;
                            break;
                        }
                    }

                    if (isInfantBee) {
                        score -= 500; // Penalty for killing Infant Bee in child form
                        // Handle Infant Bee state if necessary
                    }
                    else {
                        bees[i].isAlive = false;
                        deathCount++;

                        if (honeycombCount < maxHoneycombs) {
                            honeycombs[honeycombCount].sprite.setTexture(honeycombTexture);
                            if (bees[i].isKiller)
                                honeycombs[honeycombCount].sprite.setColor(Color::Red);
                            honeycombs[honeycombCount].sprite.setPosition(bees[i].sprite.getPosition());
                            honeycombs[honeycombCount].honeycomb_collection.restart();
                            honeycombCount++;
                        }
                        score += bees[i].isKiller ? 200 : 100;
                    }

                    bulletExists = false;
                    cout << "Display Score: " << score << endl;
                }
            }

            // Handle collisions with honeycombs
            for (int i = 0; i < honeycombCount; i++) {
                if (!honeycombs[i].isCollected &&
                    honeycombs[i].sprite.getGlobalBounds().contains(bullet_x, bullet_y) &&
                    honeycombs[i].honeycomb_collection.getElapsedTime().asSeconds() > 1.f) {
                    honeycombs[i].isCollected = true;
                    bulletExists = false;
                    score += 50;
                    break;
                }
            }
        }

        // Move bees linearly
        if (beeClock.getElapsedTime().asMilliseconds() > 50) {
            beeClock.restart();
            for (int i = 0; i < beeCount; i++) {
                if (bees[i].isAlive) {
                    Direction pos ( bees[i].sprite.getPosition().x, bees[i].sprite.getPosition().y);

                    // Worker bee random stop logic
                    if (!bees[i].isKiller) {
                        if (!bees[i].isStopped && rand() % 100 < 1) {
                            bees[i].isStopped = true;
                            bees[i].stopTimer.restart();
                        }

                        if (bees[i].isStopped && bees[i].stopTimer.getElapsedTime().asSeconds() > 2) {
                            bees[i].isStopped = false;
                        }
                    }

                    // Handle pollination and exiting
                    if (!bees[i].isStopped) {
                        if (pos.y >= resolutionY - (4 * boxPixelsY) && !bees[i].isExiting) {
                            bees[i].isExiting = true;

                            if (pos.x > resolutionX / 2) { // Right side
                                if (flowerCount < maxFlowers) {
                                    if (flowerCountRight == 0) {
                                        flowers[flowerCount].sprite.setTexture(flowerTexture);
                                        flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                        flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
                                        flowerCountRight++;
                                        flowerCount++;
                                    }
                                    flowers[flowerCount].sprite.setTexture(flowerTexture);
                                    flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                    flowers[flowerCount].sprite.setPosition(resolutionX - flowerCountRight * boxPixelsX, flowerRowStart * boxPixelsY);
                                    flowerCount++;
                                    flowerCountRight++;
                                    deathCount++;
                                }
                            }
                            else { // Left side
                                if (flowerCount < maxFlowers) {
                                    if (flowerCountLeft == 0) {
                                        flowers[flowerCount].sprite.setTexture(flowerTexture);
                                        flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                        flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
                                        flowerCountLeft++;
                                        flowerCount++;
                                    }
                                    flowers[flowerCount].sprite.setTexture(flowerTexture);
                                    flowers[flowerCount].sprite.setScale(1.f, 1.f);
                                    flowers[flowerCount].sprite.setPosition(flowerCountLeft * boxPixelsX, flowerRowStart * boxPixelsY);
                                    flowerCount++;
                                    flowerCountLeft++;
                                    deathCount++;
                                }
                            }
                        }

                        // Handle exiting bees
                        if (bees[i].isExiting) {
                            if (bees[i].sprite.getPosition().x > resolutionX / 2)
                                pos.x += bees[i].speed;
                            else
                                pos.x -= bees[i].speed;
                            if (pos.y > resolutionY) {
                                bees[i].isAlive = false;
                            }
                        }
                        else {
                            pos.x += bees[i].beeDirection * bees[i].speed;
                            if (pos.x >= resolutionX - boxPixelsX || pos.x <= 0) {
                                pos.y += boxPixelsY;
                                bees[i].beeDirection = -bees[i].beeDirection;
                            }
                        }
                    }

                    bees[i].sprite.setPosition(pos.x, pos.y);
                }
            }
        }

        // Power-Up Effects Management
        // Iterate through all power-ups and manage their effects
        // This example uses boolean flags; you can enhance it with more sophisticated state management
        // Apply effects based on active power-ups
        // Example implementation:

        // Speed Increase
        if (speedIncreased) {
            // Increase player speed
            // Modify player movement speed as needed
            // For example, change movement increment from 0.5f to 1.0f
            // Implement accordingly in movement logic
            // Here, assuming we have a variable to adjust speed
            // speedModifier += 0.5f;
            // Ensure to reset speed after duration
        }

        // Speed Decrease
        if (speedDecreased) {
            // Decrease player speed
            // Modify player movement speed as needed
            // For example, change movement increment from 0.5f to 0.25f
            // Implement accordingly in movement logic
        }

        // Height Increase
        if (heightIncreased) {
            // Increase spray can height
            // Modify spray can properties as needed
        }

        // Height Decrease
        if (heightDecreased) {
            // Decrease spray can height
            // Modify spray can properties as needed
        }

        // Reset or remove effects based on timer
        // Implement logic to check if power-up durations have expired and reset flags

        // Eating Honeycombs and Spawning Power-Ups
        // This logic has been handled earlier in power-up spawning conditions

        // Rendering
        window.clear();
        window.draw(player);

        // Draw Hives
        for (int i = 0; i < totalHives; i++) {
            window.draw(hives[i]);
        }

        // Draw Bees
        for (int i = 0; i < beeCount; i++) {
            if (bees[i].isAlive)
                window.draw(bees[i].sprite);
        }

        // Draw Infant Bees
        for (int i = 0; i < nextInfantBeeIndex; ++i) {
    if (infantBees[i].isAlive) {
        window.draw(infantBees[i].sprite);
    }
}


        // Draw Honeycombs
        for (int i = 0; i < honeycombCount; i++) {
            if (!honeycombs[i].isCollected)
                window.draw(honeycombs[i].sprite);
        }

        // Draw Flowers
        for (int i = 0; i < flowerCount; i++) {
            window.draw(flowers[i].sprite);
        }

        // Draw Power-Ups
        for (int i = 0; i < MAX_POWER_UPS; ++i) {
    if (powerUps[i].isActive) {
        window.draw(powerUps[i].sprite);
        // Draw timer bars here if implemented
    }
}


        // Draw HummingBird
        if (bird.isAlive)
            window.draw(bird.sprite);

        // Draw Bullets
        if (bulletExists) {
            window.draw(bulletSprite);
        }

        // HUD
        displayScoreAndLives(window, score, lives, sprayCount);

        window.display();
    }

    
}
// Helper Functions
void initializePowerUp(PowerUp& powerUp, Texture& texture, int type, float x, float y) {
    powerUp.sprite.setTexture(texture);
    powerUp.type = type;
    powerUp.isActive = true;
    powerUp.sprite.setPosition(x, y);
    powerUp.duration = 10.f; // Duration of 10 seconds
}

void initializeInfantBee(InfantBee& infantBee, Texture& texture, float x, float y) {
    infantBee.sprite.setTexture(texture);
    infantBee.sprite.setPosition(x, y);
    infantBee.isAlive = true;
    infantBee.isTransformed = false;
    infantBee.direction = Direction(0.f, -1.f); // Initial direction upward
}
        // Main Menu with updated options and sequential level progression
        void displayMainMenu(RenderWindow & window) {
            // Load the font once here
            if (!font.loadFromFile("ArchivoNarrow-Bold.ttf")) {
                cout << "Error loading font" << endl;
                return;
            }

            // Create text objects for the menu options
            Text title("Buzz Bombers", font, 50);
            title.setFillColor(Color::Yellow);
            title.setPosition(resolutionX / 4, resolutionY / 6);

            Text startGame("Start Game", font, 30);
            startGame.setFillColor(Color::Green);
            startGame.setPosition(resolutionX / 2 - 50, resolutionY / 2 - 80);

            Text level2("Level 2", font, 30);
            level2.setFillColor(Color::Green);
            level2.setPosition(resolutionX / 2 - 50, resolutionY / 2 - 40);

            Text level3("Level 3", font, 30);
            level3.setFillColor(Color::Green);
            level3.setPosition(resolutionX / 2 - 50, resolutionY / 2);

            Text highScoresOption("High Scores", font, 30);
            highScoresOption.setFillColor(Color::Green);
            highScoresOption.setPosition(resolutionX / 2 - 50, resolutionY / 2 + 40);

            Text bossLevel("Boss Level", font, 30);
            bossLevel.setFillColor(Color::Green);
            bossLevel.setPosition(resolutionX / 2 - 50, resolutionY / 2 + 80);

            Text exitOption("Exit", font, 30);
            exitOption.setFillColor(Color::Green);
            exitOption.setPosition(resolutionX / 2 - 50, resolutionY / 2 + 120);

            // Track the selected option
            int selectedOption = 0;
            Text* menuOptions[] = { &startGame, &level2, &level3, &highScoresOption, &bossLevel, &exitOption }; // Array of pointers to Text objects
            menuOptions[selectedOption]->setFillColor(Color::Red); // Highlight the first option

            while (window.isOpen()) {
                Event e;
                while (window.pollEvent(e)) {
                    if (e.type == Event::Closed) {
                        window.close();
                    }

                    // Handle key presses for navigating the menu
                    if (e.type == Event::KeyPressed) {
                        if (e.key.code == Keyboard::Up) {
                            // Move up
                            if (selectedOption > 0) {
                                menuOptions[selectedOption]->setFillColor(Color::Green); // Deselect current option
                                selectedOption--;
                                menuOptions[selectedOption]->setFillColor(Color::Red); // Highlight new option
                            }
                        }
                        else if (e.key.code == Keyboard::Down) {
                            // Move down
                            if (selectedOption < 5) { // 6 options: 0-5
                                menuOptions[selectedOption]->setFillColor(Color::Green); // Deselect current option
                                selectedOption++;
                                menuOptions[selectedOption]->setFillColor(Color::Red); // Highlight new option
                            }
                        }
                        else if (e.key.code == Keyboard::Enter) {
                            // Start the corresponding action
                            if (selectedOption == 0) {
                                // Start Game and sequentially run levels
                                level1Game(window);
                                level2Game(window);
                                level3Game(window);
                                gameOver(window, score);
                                // Reset game variables if needed
                                score = 0;
                                lives = 3;
                                sprayCount = 56;
                            }
                            else if (selectedOption == 1) {
                                level2Game(window); // Start level 2 game
                                level3Game(window); // Start level 2 game
                            }
                            else if (selectedOption == 2) {
                                level3Game(window); // Start level 3 game
                            }
                            else if (selectedOption == 3) {
                                // View high scores
                                showHighScores(window, font);
                            }
                            else if (selectedOption == 4) {
                                // Boss Level - Not implemented yet
                                // For now, do nothing or display a message
                                level4Game(window);
                                // Alternatively, display a temporary message on screen
                                // For simplicity, just ignore
                            }
                            else if (selectedOption == 5) {
                                // Exit the game
                                window.close();
                            }
                        }
                    }
                }

                // Draw the menu
                window.clear();
                window.draw(title);
                window.draw(startGame);
                window.draw(level2);
                window.draw(level3);
                window.draw(highScoresOption);
                window.draw(bossLevel);
                window.draw(exitOption);
                window.display();
            }
        }

        int main() {
            srand(time(NULL)); // Initialize random seed
            RenderWindow window(VideoMode(resolutionX, resolutionY), "Buzz Bombers");
             sf::Music backgroundMusic;
    if (!backgroundMusic.openFromFile("Music/magical_journey.ogg")) {
        std::cerr << "Error: Could not load background music!" << std::endl;
        return -1;
    }

    // Set the music to loop (optional)
    backgroundMusic.setLoop(true);

    // Play the background music
    backgroundMusic.play();
            displayMainMenu(window);  // Display the main menu
            return 0;
        }
