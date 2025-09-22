#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

using namespace sf;

// Game constants
const int width = 1024;
const int height = 768;
const int roadW = 2000;
const int segL = 200; // segment length
const float camD = 0.84f; // camera depth
const int N_LINES = 1600;
const int TOTAL_LAPS = 8; // Race ends after 8 laps
const float desiredCarHeight = 150.0f; // Moved to global scope for consistency; was 150

// Draw window
void drawQuad(RenderWindow& w, Color c, int x1, int y1, int w1, int x2, int y2, int w2)
{
    ConvexShape shape(4);
    shape.setFillColor(c);
    shape.setPoint(0, Vector2f(static_cast<float>(x1 - w1), static_cast<float>(y1)));
    shape.setPoint(1, Vector2f(static_cast<float>(x2 - w2), static_cast<float>(y2)));
    shape.setPoint(2, Vector2f(static_cast<float>(x2 + w2), static_cast<float>(y2)));
    shape.setPoint(3, Vector2f(static_cast<float>(x1 + w1), static_cast<float>(y1)));
    w.draw(shape);
}

// Draw lines
struct Line
{
    float x, y, z; // 3d center of line
    float X, Y, W; // screen coord
    float curve, spriteX, clip, scale;
    Sprite sprite;
    bool isFinishLine; // Indica se é parte da linha de chegada

    Line()
    {
        spriteX = curve = x = y = z = 0.0f;
        isFinishLine = false;
    }

    void project(int camX, int camY, int camZ)
    {
        scale = camD / (z - static_cast<float>(camZ));
        X = (1.0f + scale * (x - static_cast<float>(camX))) * width / 2.0f;
        Y = (1.0f - scale * (y - static_cast<float>(camY))) * height / 2.0f;
        W = scale * roadW * width / 2.0f;
    }

    void drawSprite(RenderWindow& app)
    {
        Sprite s = sprite;
        int w = s.getTextureRect().width;
        int h = s.getTextureRect().height;

        float destX = X + scale * spriteX * width / 2.0f;
        float destY = Y + 4.0f;
        float destW = static_cast<float>(w) * W / 266.0f;
        float destH = static_cast<float>(h) * W / 266.0f;

        destX += destW * spriteX; // offsetX
        destY += destH * (-1.0f); // offsetY

        float clipH = destY + destH - clip;
        if (clipH < 0.0f) clipH = 0.0f;

        if (clipH >= destH) return;
        s.setTextureRect(IntRect(0, 0, w, static_cast<int>(h - h * clipH / destH)));
        s.setScale(destW / w, destH / h);
        s.setPosition(destX, destY);
        app.draw(s);
    }
};

// Opponent structure
struct Opponent
{
    float pos; // Position along track
    float opponentX; // Lateral position
    float speed; // Current speed
    float baseSpeed; // Base speed (constant)
    int laps; // Laps completed
    Sprite sprite; // Opponent car sprite
    bool finished; // Whether opponent has finished the race
    float targetX; // Target lateral position for smoother movement

    Opponent(float startPos, float x, float spd, Texture& tex)
        : pos(startPos), opponentX(x), speed(spd), baseSpeed(spd), laps(0), finished(false), targetX(x)
    {
        sprite.setTexture(tex);
    }

    void update(float elapsedSeconds, std::vector<Line>& lines, bool raceStarted)
    {
        if (!raceStarted || finished) return;

        // Get current segment and track properties
        int currentSegment = static_cast<int>(pos / segL) % N_LINES;
        float currentCurve = lines[currentSegment].curve;

        // Move opponent
        pos += speed * elapsedSeconds * 125.0f; // Match player's speed scaling
        std::cout << "Opponent pos: " << pos << ", segment: " << currentSegment << ", laps: " << laps << std::endl;
        while (pos >= N_LINES * segL) {
            pos -= N_LINES * segL;
            laps++;
            if (laps >= TOTAL_LAPS) {
                finished = true;
                std::cout << "Opponent finished race!" << std::endl;
            }
        }

        // Update targetX to follow the inside of curves
        targetX = -currentCurve * 0.5f; // Move toward inside of curve
        float maxOpponentX = 0.8f; // Tighter limit to stay on road
        if (targetX > maxOpponentX) targetX = maxOpponentX;
        if (targetX < -maxOpponentX) targetX = -maxOpponentX;

        // Smoothly adjust opponentX toward targetX
        float steerSpeed = 0.3f; // Rate of lateral movement
        opponentX += (targetX - opponentX) * steerSpeed * elapsedSeconds;

        // Limit opponentX to stay on road
        if (opponentX > maxOpponentX) opponentX = maxOpponentX;
        if (opponentX < -maxOpponentX) opponentX = -maxOpponentX;
    }

    void draw(RenderWindow& app, int playerPos, int camH, std::vector<Line>& lines)
    {
        if (finished) {
            std::cout << "Opponent finished, not drawing." << std::endl;
            return;
        }

        int opponentSegment = static_cast<int>(pos / segL) % N_LINES;
        Line& l = lines[opponentSegment];
        float relativeZ = l.z - (playerPos % (N_LINES * segL));
        if (relativeZ < 0) relativeZ += N_LINES * segL;
        std::cout << "Opponent segment: " << opponentSegment << ", relativeZ: " << relativeZ << std::endl;

        // Remover restrição de visibilidade para teste
        if (relativeZ < 10 || relativeZ > segL * 100) {
             std::cout << "Opponent out of range: relativeZ = " << relativeZ << std::endl;
             return;
         }

        // Usar projeção semelhante à Line::drawSprite
        float scale = camD / std::max(relativeZ, 1.0f);
        float destX = l.X + scale * opponentX * width / 2.0f;
        float destY = l.Y + 4.0f;

        Sprite s = sprite;
        int w = s.getTextureRect().width;
        int h = s.getTextureRect().height;

        float baseScale = desiredCarHeight / static_cast<float>(h); // Mesma altura base do jogador
        float distanceScale = std::max(0.1f, (1.0f / relativeZ) * 2400.0f);
        float finalScale = std::max(0.1f, std::min(baseScale * distanceScale, 5.0f)); // Limitar escala - 1
        float destW = static_cast<float>(w) * finalScale;
        float destH = static_cast<float>(h) * finalScale;

        destX += destW * opponentX; // offsetX
        destY -= destH; // offsetY para alinhar com a pista

        s.setScale(destW / w, destH / h);
        s.setPosition(destX, destY);
        app.draw(s);
        std::cout << "Drawing opponent at X: " << destX << ", Y: " << destY << ", Scale: " << (destW / w)
            << ", relativeZ: " << relativeZ << ", opponentX: " << opponentX
            << ", w: " << w << ", h: " << h << std::endl;
    }
};

// Função para mostrar tela de introdução e contagem regressiva
void showIntroScreen(RenderWindow& app, Font& font) {
    Text introText("", font, 30);
    introText.setFillColor(Color::White);

    std::vector<std::string> lines = {
        "TopGear Racing!",
        "How to play:",
        "W: Acceleration",
        "A: Turn left",
        "D: Turn right",
        "S: Brake",
        "Arrow Up: Up gear",
        "Arrow Down: Down Gear",
        "Press Enter to Start!"
    };

    bool waiting = true;
    while (waiting && app.isOpen()) {
        Event e;
        while (app.pollEvent(e)) {
            if (e.type == Event::Closed)
                app.close();
        }

        if (Keyboard::isKeyPressed(Keyboard::Enter)) {
            waiting = false;
            break;
        }

        app.clear(Color::Black);

        for (size_t i = 0; i < lines.size(); i++) {
            introText.setString(lines[i]);
            introText.setPosition(50.f, 50.f + i * 50.f);
            app.draw(introText);
        }

        app.display();
    }

    // Contador regressivo 3..2..1
    Clock clock;
    for (int count = 3; count > 0; count--) {
        float elapsed = 0.f;
        while (elapsed < 1.0f && app.isOpen()) {
            Event e;
            while (app.pollEvent(e)) {
                if (e.type == Event::Closed)
                    app.close();
            }

            elapsed = clock.getElapsedTime().asSeconds();

            app.clear(Color::Black);
            introText.setString(std::to_string(count));
            introText.setCharacterSize(120);
            introText.setPosition(width / 2.f - 50.f, height / 2.f - 60.f);
            app.draw(introText);
            app.display();
        }
        clock.restart();
    }
}

// Função para mostrar tela de resultado final
void showResultScreen(RenderWindow& app, Font& font, int playerPosition) {
    Text resultText("", font, 50);
    resultText.setFillColor(Color::White);
    resultText.setOutlineColor(Color::Black);
    resultText.setOutlineThickness(2.f);
    std::string positionStr = (playerPosition == 1) ? "1st" : (playerPosition == 2) ? "2nd" : "3rd";
    resultText.setString("Race Finished!\nYour Position: " + positionStr);
    resultText.setPosition(width / 2.f - 400.f, height / 2.f - 50.f);

    Clock clock;
    float elapsed = 0.f;
    while (elapsed < 5.0f && app.isOpen()) {
        Event e;
        while (app.pollEvent(e)) {
            if (e.type == Event::Closed)
                app.close();
        }

        elapsed = clock.getElapsedTime().asSeconds();

        app.clear(Color::Black);
        app.draw(resultText);
        app.display();
    }
}

int main()
{
    RenderWindow app(VideoMode(width, height), "TopGear Racing!");
    app.setFramerateLimit(60);

    Texture t[50];
    Sprite object[50];
    for (int i = 1; i <= 7; i++)
    {
        std::string filename = "images/" + std::to_string(i) + ".png";
        if (!t[i].loadFromFile(filename)) {
            std::cerr << "Failed to load image: " << filename << std::endl;
            return -1;
        }
        t[i].setSmooth(true);
        object[i].setTexture(t[i]);
    }

    Texture bg;
    if (!bg.loadFromFile("images/bg.png")) {
        std::cerr << "Failed to load background image." << std::endl;
        return -1;
    }
    bg.setRepeated(true);
    Sprite sBackground(bg);
    sBackground.setTextureRect(IntRect(0, 0, 5000, 411));
    sBackground.setPosition(-2000.0f, 0.0f);

    // Carregar texturas dos adversários
    Texture blueOpponentTexture, yellowOpponentTexture;
    if (!blueOpponentTexture.loadFromFile("images/blue_car.png")) {
        std::cerr << "Failed to load blue car texture." << std::endl;
        return -1;
    }
    if (!yellowOpponentTexture.loadFromFile("images/yellow_car.png")) {
        std::cerr << "Failed to load yellow car texture." << std::endl;
        return -1;
    }


    std::vector<Line> lines;
    for (int i = 0; i < N_LINES; i++)
    {
        Line line;
        line.z = static_cast<float>(i * segL);

        if (i > 300 && i < 700) line.curve = 0.5f;
        if (i > 1100) line.curve = -0.7f;

        if (i < 300 && i % 20 == 0) { line.spriteX = -2.5f; line.sprite = object[5]; }
        if (i % 17 == 0) { line.spriteX = 2.0f; line.sprite = object[6]; }
        if (i > 300 && i % 20 == 0) { line.spriteX = -0.7f; line.sprite = object[4]; }
        if (i > 800 && i % 20 == 0) { line.spriteX = -1.2f; line.sprite = object[1]; }
        if (i == 400) { line.spriteX = -1.2f; line.sprite = object[7]; }

        if (i > 750) line.y = sin(i / 30.0f) * 1500.0f;

        if (i >= 0 && i < 10) line.isFinishLine = true;

        lines.push_back(line);
    }

    float playerX = 0.0f;
    int pos = (N_LINES - 20) * segL; // Inicia o carro 20 segmentos antes da linha de chegada
    const int H = 900;
    float speed = 0.0f;
    int lapsCompleted = 0;
    float carGas = 100.0f;
    bool raceStarted = false;
    bool playerFinished = false;

    int gear = 1;
    const int maxGear = 5;
    float gearMaxSpeed[] = { 0, 30 * 3, 60 * 3, 90 * 3, 110 * 3, 133 * 3 };
    float gearAcceleration[] = { 0, 12.0f, 10.0f, 7.0f, 5.0f, 3.0f };

    // Inicializar adversários mais próximos com texturas diferentes
    std::vector<Opponent> opponents = {
        Opponent((N_LINES - 4) * segL, -0.8f, 200.0f, blueOpponentTexture), // old velocity was 100 and 110
        Opponent((N_LINES - 2) * segL,  0.8f, 220.0f, yellowOpponentTexture)
    };

    Font font;
    if (!font.loadFromFile("fonts/PressStart2P-Regular.ttf")) {
        std::cerr << "Failed to load font." << std::endl;
        return -1;
    }

    // Tela de introdução e contagem regressiva
    showIntroScreen(app, font);

    // Criar textos HUD
    Text velocityText("", font, 30);
    velocityText.setFillColor(Color::White);
    velocityText.setPosition(20.0f, 100.0f);
    velocityText.setOutlineColor(Color::Black);
    velocityText.setOutlineThickness(2.f);

    Text gasText("", font, 30);
    gasText.setFillColor(Color::White);
    gasText.setPosition(20.0f, 300.0f);
    gasText.setOutlineColor(Color::Black);
    gasText.setOutlineThickness(2.f);

    Text lapCounterText("", font, 30);
    lapCounterText.setFillColor(Color::White);
    lapCounterText.setPosition(20.0f, 60.0f);
    lapCounterText.setOutlineColor(Color::Black);
    lapCounterText.setOutlineThickness(2.f);

    Text gearText("", font, 30);
    gearText.setFillColor(Color::Yellow);
    gearText.setPosition(20.0f, 140.0f);
    gearText.setOutlineColor(Color::Black);
    gearText.setOutlineThickness(2.f);

    Text grassText("", font, 30);
    grassText.setFillColor(Color::Red);
    grassText.setPosition(20.0f, 340.0f);
    grassText.setOutlineColor(Color::Black);
    grassText.setOutlineThickness(2.f);

    Text positionText("", font, 30);
    positionText.setFillColor(Color::Cyan);
    positionText.setPosition(20.0f, 180.0f);
    positionText.setOutlineColor(Color::Black);
    positionText.setOutlineThickness(2.f);

    Clock clock;
    float elapsedSeconds = 0.0f;
    int lastStartPos = (N_LINES - 20);
    bool isOnGrass = false;

    // Carro do jogador
    Texture carTexture, carLeftTexture, carRightTexture;
    if (!carTexture.loadFromFile("images/car.png") ||
        !carLeftTexture.loadFromFile("images/car_left.png") ||
        !carRightTexture.loadFromFile("images/car_right.png")) {
        std::cerr << "Failed to load car textures." << std::endl;
        return -1;
    }

    Sprite carSprite(carTexture);
    auto setCarTexture = [&](Texture& tex, float extraHeightScale = 1.0f) {
        carSprite.setTexture(tex, true);
        float scaleY = desiredCarHeight / carSprite.getLocalBounds().height;
        scaleY *= extraHeightScale;
        float scaleX = scaleY;
        carSprite.setScale(scaleX, scaleY);
        carSprite.setPosition(width / 2.f - carSprite.getGlobalBounds().width / 2.0f,
            height * 0.7f);
        };
    setCarTexture(carTexture);

    bool canShiftDown = true;

    while (app.isOpen()) {
        Event e;
        while (app.pollEvent(e)) {
            if (e.type == Event::Closed) app.close();
        }

        elapsedSeconds = clock.restart().asSeconds();

        // Iniciar corrida na primeira pressão de W
        if (Keyboard::isKeyPressed(Keyboard::W) && !raceStarted) {
            raceStarted = true;
            std::cout << "Race Started!" << std::endl;
        }

        // Atualizar adversários
        for (auto& opponent : opponents) {
            opponent.update(elapsedSeconds, lines, raceStarted);
        }

        // Get the current segment's curve
        int currentSegment = pos / segL;
        float currentCurve = lines[currentSegment % N_LINES].curve;

        // Curve influence on playerX
        float curveInfluence = currentCurve * elapsedSeconds * (speed / 200.0f);
        playerX += curveInfluence;

        // Steering input (A: move left, D: move right)
        float steeringForce = 0.6f;
        if (speed >= 50) {
            if (Keyboard::isKeyPressed(Keyboard::A)) {
                playerX -= steeringForce * elapsedSeconds;
                setCarTexture(carLeftTexture, 1.25f);
            }
            else if (Keyboard::isKeyPressed(Keyboard::D)) {
                playerX += steeringForce * elapsedSeconds;
                setCarTexture(carRightTexture, 1.25f);
            }
            else {
                setCarTexture(carTexture);
            }
        }
        else {
            setCarTexture(carTexture);
        }

        // Verificar se o carro está na grama
        isOnGrass = (std::abs(playerX * roadW) > roadW / 2.0f * 1.2f);
        if (isOnGrass) {
            std::cout << "On Grass! Speed: " << speed / 3 << " km/h, Gear: " << gear << ", Gas: " << carGas << std::endl;
            speed -= 0.3f * elapsedSeconds;
            if (speed < 0) speed = 0;
        }

        // Aceleração
        if (Keyboard::isKeyPressed(Keyboard::W) && carGas > 0 && !playerFinished) {
            float currentAcceleration = isOnGrass ? gearAcceleration[gear] * 0.8f : gearAcceleration[gear];
            speed += currentAcceleration * elapsedSeconds;
            float currentMaxSpeed = isOnGrass ? gearMaxSpeed[gear] * 0.8f : gearMaxSpeed[gear];
            if (speed > currentMaxSpeed) speed = currentMaxSpeed;
            std::cout << "Accelerating! Speed: " << speed / 3 << " km/h, Gear: " << gear << ", Gas: " << carGas << std::endl;
        }
        else {
            speed -= 0.5f * elapsedSeconds;
            if (speed < 0) speed = 0;
        }

        // Limit playerX
        float maxPlayerX = 2.0f;
        if (playerX > maxPlayerX) playerX = maxPlayerX;
        if (playerX < -maxPlayerX) playerX = -maxPlayerX;

        // Mudança de marchas
        if (Keyboard::isKeyPressed(Keyboard::Up)) {
            if (gear < maxGear && static_cast<int>(speed) >= gearMaxSpeed[gear] * 0.8f) {
                gear++;
                float currentMaxSpeed = isOnGrass ? gearMaxSpeed[gear] * 0.8f : gearMaxSpeed[gear];
                if (speed > currentMaxSpeed) speed = currentMaxSpeed;
                std::cout << "Upshifted to Gear: " << gear << std::endl;
            }
        }
        if (Keyboard::isKeyPressed(Keyboard::Down)) {
            if (canShiftDown && gear > 1) {
                gear--;
                std::cout << "Downshifted to Gear: " << gear << std::endl;
                canShiftDown = false;
            }
        }
        else canShiftDown = true;

        // Atualizar posição do jogador
        if (!playerFinished) {
            pos += static_cast<int>(speed * elapsedSeconds * 125.0f);
            while (pos >= N_LINES * segL) {
                pos -= N_LINES * segL;
                lapsCompleted++;
                if (lapsCompleted >= TOTAL_LAPS) {
                    playerFinished = true;
                    std::cout << "Player finished race!" << std::endl;
                }
            }
            while (pos < 0) pos += N_LINES * segL;
        }

        int startPos = pos / segL;
        int camH = static_cast<int>(lines[startPos].y + H);
        if (speed > 0) sBackground.move(-lines[startPos].curve * 2.f * elapsedSeconds * 5.0f, 0.f);
        if (speed < 0) sBackground.move(lines[startPos].curve * 2.f * elapsedSeconds * 5.0f, 0.f);

        // Contagem de voltas
        int newPos = pos / segL;
        if (lastStartPos >= N_LINES - 20 && newPos <= 9 && lines[newPos].isFinishLine) {
            std::cout << "Lap completed! Total laps: " << lapsCompleted << std::endl;
        }
        lastStartPos = newPos;

        // Calcular posição na corrida
        std::vector<std::pair<float, int>> rankings; // {distância total, índice (0=player, 1=opponent1, 2=opponent2)}
        rankings.push_back({ static_cast<float>(lapsCompleted * N_LINES * segL + pos), 0 });
        rankings.push_back({ static_cast<float>(opponents[0].laps * N_LINES * segL + opponents[0].pos), 1 });
        rankings.push_back({ static_cast<float>(opponents[1].laps * N_LINES * segL + opponents[1].pos), 2 });
        std::sort(rankings.rbegin(), rankings.rend()); // Ordem decrescente

        int playerPosition = 1;
        for (size_t i = 0; i < rankings.size(); i++) {
            if (rankings[i].second == 0) {
                playerPosition = i + 1;
                break;
            }
        }
        positionText.setString("Position: " + std::to_string(playerPosition));

        // Atualiza HUD
        velocityText.setString("Velocity: " + std::to_string(static_cast<int>(speed) / 3) + " km/h");
        gasText.setString("Gas: " + std::to_string(static_cast<int>(carGas)));
        lapCounterText.setString("Laps: " + std::to_string(lapsCompleted) + "/" + std::to_string(TOTAL_LAPS));
        gearText.setString("Gear: " + std::to_string(gear));
        grassText.setString(isOnGrass ? "On Grass!" : "");

        // Consumo de combustível
        if (speed > 0 && carGas > 0) {
            carGas -= ((speed / 20.0f) * elapsedSeconds / 6.f) * static_cast<float>(gear);
            if (carGas < 0) carGas = 0;
        }
        if (carGas <= 0) {
            speed -= 0.5f * elapsedSeconds;
            if (speed < 0) speed = 0;
            std::cout << "Out of Gas!" << std::endl;
        }

        app.clear(Color(105, 205, 4));
        app.draw(sBackground);

        int maxy = height; float x = 0.f, dx = 0.f;

        // Desenhar a pista
        for (int n = startPos; n < startPos + 300; n++) {
            Line& l = lines[n % N_LINES];
            l.project(static_cast<int>(playerX * roadW - x), camH, startPos * segL - (n >= N_LINES ? N_LINES * segL : 0));

            if (l.sprite.getTexture() == object[7].getTexture()) {
                float spriteTop = l.Y + 4.f - (l.W * l.sprite.getLocalBounds().height / 266.f);
                float spriteBottom = l.Y + 4.f;
                float carTop = carSprite.getPosition().y;
                float carBottom = carTop + carSprite.getGlobalBounds().height;
                if (carBottom > spriteTop && carTop < spriteBottom) {
                    carGas = carGas + 2.0f;
                    std::cout << "Collected Gas! Gas: " << carGas << std::endl;
                }
            }

            x += dx;
            dx += l.curve;

            l.clip = static_cast<float>(maxy);
            if (l.Y >= maxy) continue;
            maxy = static_cast<int>(l.Y);

            Color grass = (n / 3) % 2 ? Color(16, 200, 16) : Color(0, 154, 0);
            Color rumble = l.isFinishLine ? Color::Black : ((n / 3) % 2 ? Color(255, 255, 255) : Color(0, 0, 0));
            Color road = l.isFinishLine ? Color::White : ((n / 3) % 2 ? Color(107, 107, 107) : Color(105, 105, 105));

            Line p = lines[(n - 1) % N_LINES];
            drawQuad(app, grass, 0, p.Y, width, 0, l.Y, width);
            drawQuad(app, rumble, static_cast<int>(p.X), static_cast<int>(p.Y), static_cast<int>(p.W * 1.2f),
                static_cast<int>(l.X), static_cast<int>(l.Y), static_cast<int>(l.W * 1.2f));
            drawQuad(app, road, static_cast<int>(p.X), static_cast<int>(p.Y), static_cast<int>(p.W),
                static_cast<int>(l.X), static_cast<int>(l.Y), static_cast<int>(l.W));
        }

        // Desenhar sprites da pista (de trás para frente)
        for (int n = startPos + 300; n > startPos; n--) {
            lines[n % N_LINES].drawSprite(app);
        }

        // Desenhar adversários após a pista, mas antes do carro do jogador
        for (auto& opponent : opponents) {
            opponent.draw(app, pos, camH, lines);
        }

        app.draw(carSprite);
        app.draw(lapCounterText);
        app.draw(velocityText);
        app.draw(gearText);
        app.draw(gasText);
        app.draw(grassText);
        app.draw(positionText);

        // Verificar fim da corrida
        if (playerFinished) {
            showResultScreen(app, font, playerPosition);
            break; // Sai do loop principal após mostrar o resultado
        }

        app.display();
    }

    return 0;
}