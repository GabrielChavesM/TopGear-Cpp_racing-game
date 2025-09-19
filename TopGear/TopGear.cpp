#include <SFML/Graphics.hpp>
#include <iostream>

using namespace sf;

int width = 1024;
int height = 768;
int roadW = 2000;
int segL = 200; //segment length
float camD = 0.84; //camera depth


// Draw window
void drawQuad(RenderWindow& w, Color c, int x1, int y1, int w1, int x2, int y2, int w2)
{
    ConvexShape shape(4);
    shape.setFillColor(c);
    shape.setPoint(0, Vector2f(x1 - w1, y1));
    shape.setPoint(1, Vector2f(x2 - w2, y2));
    shape.setPoint(2, Vector2f(x2 + w2, y2));
    shape.setPoint(3, Vector2f(x1 + w1, y1));
    w.draw(shape);
}


// Draw lines
struct Line
{
    float x, y, z; //3d center of line
    float X, Y, W; //screen coord
    float curve, spriteX, clip, scale;
    Sprite sprite;

    Line()
    {
        spriteX = curve = x = y = z = 0;
    }

    void project(int camX, int camY, int camZ)
    {
        scale = camD / (z - camZ);
        X = (1 + scale * (x - camX)) * width / 2;
        Y = (1 - scale * (y - camY)) * height / 2;
        W = scale * roadW * width / 2;
    }

    void drawSprite(RenderWindow& app)
    {
        Sprite s = sprite;
        int w = s.getTextureRect().width;
        int h = s.getTextureRect().height;

        float destX = X + scale * spriteX * width / 2;
        float destY = Y + 4;
        float destW = w * W / 266;
        float destH = h * W / 266;

        destX += destW * spriteX; //offsetX
        destY += destH * (-1);    //offsetY

        float clipH = destY + destH - clip;
        if (clipH < 0) clipH = 0;

        if (clipH >= destH) return;
        s.setTextureRect(IntRect(0, 0, w, h - h * clipH / destH));
        s.setScale(destW / w, destH / h);
        s.setPosition(destX, destY);
        app.draw(s);
    }
};


int lapsCompleted = 0; // Lap counter
int lastStartPos = 0;
int carGas = 100;


////// MAIN FUNTION //////

int main()
{
    RenderWindow app(VideoMode(width, height), "Outrun Racing!");
    app.setFramerateLimit(60);

    Texture t[50];
    Sprite object[50];
    for (int i = 1;i <= 7;i++)
    {
        t[i].loadFromFile("images/" + std::to_string(i) + ".png");
        t[i].setSmooth(true);
        object[i].setTexture(t[i]);
    }

    Texture bg;
    bg.loadFromFile("images/bg.png");
    bg.setRepeated(true);
    Sprite sBackground(bg);
    sBackground.setTextureRect(IntRect(0, 0, 5000, 411));
    sBackground.setPosition(-2000, 0);

    std::vector<Line> lines;

    for (int i = 0;i < 1600;i++)
    {
        Line line;
        line.z = i * segL;

        if (i > 300 && i < 700) line.curve = 0.5;
        if (i > 1100) line.curve = -0.7;

        if (i < 300 && i % 20 == 0) { line.spriteX = -2.5; line.sprite = object[5]; }
        if (i % 17 == 0) { line.spriteX = 2.0; line.sprite = object[6]; }
        if (i > 300 && i % 20 == 0) { line.spriteX = -0.7; line.sprite = object[4]; }
        if (i > 800 && i % 20 == 0) { line.spriteX = -1.2; line.sprite = object[1]; }
        if (i == 400) { line.spriteX = -1.2; line.sprite = object[7]; }

        if (i > 750) line.y = sin(i / 30.0) * 1500;

        lines.push_back(line);
    }

    int N = lines.size();
    float playerX = 0;
    int pos = 0;
    int H = 900;

    // Load the font for velocity text
    Font font;
    if (!font.loadFromFile("fonts/arial.ttf"))
    {
        std::cerr << "Failed to load font." << std::endl;
        return -1;
    }

    // Load font for gas text
    Font font;
    if (!font.loadFromFile("fonts/arial.ttf")) {
        std::cerr << "Failed to load font." << std::endl;
        return -1;
    }

    // Create the velocity text
    Text velocityText("", font, 30);
    velocityText.setFillColor(Color::White);
    velocityText.setPosition(20, 100); // Update the Y position to display at the bottom of the screen

    // Create the gas text
    Text gasText("", font, 30);
    gasText.setFillColor(Color::White);
    gasText.setPosition(20, 300);

    // Initialize the clock
    Clock clock;
    float elapsedSeconds = 0.0f;
    float accelSeconds = 0.0f;

    // Load the car texture
    Texture carTexture;
    if (!carTexture.loadFromFile("images/car.png")) {
        std::cerr << "Failed to load the car texture." << std::endl;
        return -1;
    }

    // Create the car sprite
    Sprite carSprite(carTexture);

    // Set the initial position of the car sprite
    const float carWidth = 250.0f;
    const float carHeight = 138.0f;
    carSprite.setPosition(384, 512);


    while (app.isOpen())
    {
        Event e;
        while (app.pollEvent(e))
        {
            if (e.type == Event::Closed)
                app.close();
        }

        int speed;

        // Calculate acceleration and deceleration
        float acceleration = 5.0f * elapsedSeconds/2;

        if (Keyboard::isKeyPressed(Keyboard::D)) playerX += 0.1;
        if (Keyboard::isKeyPressed(Keyboard::A)) playerX -= 0.1;
        if (Keyboard::isKeyPressed(Keyboard::W)) {
            speed += 1;
        }
        if (!Keyboard::isKeyPressed(Keyboard::W)) {
            speed = speed - acceleration * elapsedSeconds;
        }

        if (Keyboard::isKeyPressed(Keyboard::Space)) {
            speed *= 1.75;
            if (speed >= 500) {
                speed = 500;
            }
        }

        if (speed == 0) {
            speed = 0;
        }

        pos += speed;
        while (pos >= N * segL) pos -= N * segL;
        while (pos < 0) pos += N * segL;

        // Get the elapsed time since the last frame
        elapsedSeconds = clock.restart().asSeconds();
        accelSeconds = clock.getElapsedTime().asSeconds();

        // Limit the speed
        float maxSpeed = 500.0f; // Adjust this value to set the maximum speed
        if (speed > maxSpeed)
            speed = maxSpeed;
        else if (speed < -maxSpeed)
            speed = -maxSpeed;

        app.clear(Color(105, 205, 4));
        app.draw(sBackground);
        int startPos = pos / segL;
        int camH = lines[startPos].y + H;
        if (speed > 0) sBackground.move(-lines[startPos].curve * 2, 0);
        if (speed < 0) sBackground.move(lines[startPos].curve * 2, 0);

        // Update gas
        if (speed >= 0) {
            carGas = carGas - (speed / 100);
        }

        int maxy = height;
        float x = 0, dx = 0;

        int newPos = pos / segL; // Calculate the new pos segment

        if (newPos < lastStartPos) {
            lapsCompleted++;
        }
        lastStartPos = newPos; // Update the last start position

        Font font;
        if (!font.loadFromFile("fonts/arial.ttf")) {
            std::cerr << "Failed to load the font." << std::endl;
            return -1;
        }

        // Create the lap counter text
        Text lapCounterText("", font, 30);
        lapCounterText.setFillColor(Color::White);
        lapCounterText.setPosition(20, 60);

        //Update lap counter
        lapCounterText.setString("Laps: " + std::to_string(lapsCompleted));

        // Update the velocity text
        velocityText.setString("Velocity: " + std::to_string(static_cast<int>(speed/2.5)) + " km/h");

        // Update the gas text
        gasText.setString("Gas: " + std::to_string(carGas));


        ///////draw road////////
        for (int n = startPos; n < startPos + 300; n++)
        {
            Line& l = lines[n % N];
            l.project(playerX * roadW - x, camH, startPos * segL - (n >= N ? N * segL : 0));
            x += dx;
            dx += l.curve;

            l.clip = maxy;
            if (l.Y >= maxy) continue;
            maxy = l.Y;

            Color grass = (n / 3) % 2 ? Color(16, 200, 16) : Color(0, 154, 0);
            Color rumble = (n / 3) % 2 ? Color(255, 255, 255) : Color(0, 0, 0);
            Color road = (n / 3) % 2 ? Color(107, 107, 107) : Color(105, 105, 105);

            Line p = lines[(n - 1) % N]; //previous line

            drawQuad(app, grass, 0, p.Y, width, 0, l.Y, width);
            drawQuad(app, rumble, p.X, p.Y, p.W * 1.2, l.X, l.Y, l.W * 1.2);
            drawQuad(app, road, p.X, p.Y, p.W, l.X, l.Y, l.W);
        }

        ////////draw objects////////
        for (int n = startPos + 300; n > startPos; n--) {
            lines[n % N].drawSprite(app);
        }
        app.draw(carSprite);
        app.draw(lapCounterText);
        app.draw(velocityText);
        app.draw(gasText);
        app.display();
    }

    return 0;
}
