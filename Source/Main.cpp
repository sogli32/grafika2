#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"
#include <cstdio>


int selectedIndex = -1;
bool mouseWasDown = false; // bolja detekcija klika

void preprocessTexture(unsigned& texture, const char* filepath)
{
    texture = loadImageToTexture(filepath);
    glBindTexture(GL_TEXTURE_2D, texture);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
void DrawRect(float cx, float cy, float w, float h, float r, float g, float b,
    unsigned shader, unsigned vao)
{
    glUseProgram(shader);

    glUniform4f(glGetUniformLocation(shader, "uColor"), r, g, b, 1.0f);
    glUniform2f(glGetUniformLocation(shader, "uPos"), cx, cy);
    glUniform2f(glGetUniformLocation(shader, "uSize"), w, h);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}


void DrawTexture(float cx, float cy, float w, float h, unsigned tex, unsigned shader, unsigned vao)
{
    glUseProgram(shader);

    glUniform2f(glGetUniformLocation(shader, "uCenter"), cx, cy);
    glUniform2f(glGetUniformLocation(shader, "uScale"), w, h);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}


void getMouseNDC(GLFWwindow* window, double& outX, double& outY)
{
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    outX = (mx / width) * 2.0 - 1.0;
    outY = 1.0 - (my / height) * 2.0;
}



int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // FULLSCREEN
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(
        mode->width, mode->height,
        "Grid Demo",
        monitor, NULL
    );

    if (!window)
        return endProgram("Prozor nije uspeo da se kreira.");

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
        return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.2f, 0.8f, 0.6f, 1.0f);
    glLineWidth(3.0f);

    // ---------------------------------------------------------
    // PANEL: X od -0.9 do -0.1, Y ostaje -0.9 do 0.9
    // ---------------------------------------------------------
    float panelLeft = -0.9f;
    float panelRight = -0.1f;
    float panelTop = 0.9f;
    float panelBottom = -0.9f;

    float panelVerts[] = {
        panelLeft,  panelTop,
        panelLeft,  panelBottom,
        panelRight, panelBottom,
        panelRight, panelTop
    };

    unsigned int panelVAO, panelVBO;
    glGenVertexArrays(1, &panelVAO);
    glGenBuffers(1, &panelVBO);

    glBindVertexArray(panelVAO);
    glBindBuffer(GL_ARRAY_BUFFER, panelVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(panelVerts), panelVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float liftVerts[] = {
       0.85f, -0.80f,   // gore levo
       0.85f, -0.99f,    // dole levo
       1.0f,  -0.99f,    // dole desno
       1.0f,  -0.80f    // gore desno
    };

    unsigned int liftVAO, liftVBO;
    glGenVertexArrays(1, &liftVAO);
    glGenBuffers(1, &liftVBO);

    glBindVertexArray(liftVAO);
    glBindBuffer(GL_ARRAY_BUFFER, liftVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(liftVerts), liftVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ---------------------------------------------------------
// DOOR VAO/VBO
// ---------------------------------------------------------
    unsigned int doorVAO, doorVBO;
    glGenVertexArrays(1, &doorVAO);
    glGenBuffers(1, &doorVBO);

    glBindVertexArray(doorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, doorVBO);
    // inicijalno punjenje (dummy)
    float doorVertsInit[] = {
        0.85f, -0.80f,
        0.85f, -0.99f,
        1.0f,  -0.99f,
        1.0f,  -0.80f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(doorVertsInit), doorVertsInit, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);



    float lineVerts[] = {
        0.0f,  1.0f,   // gornja tačka
        0.0f, -1.0f    // donja tačka
    };

    unsigned int lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVerts), lineVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);



    // ---------------------------------------------------------
    // TEXTURED QUAD
    // ---------------------------------------------------------
    float quadVerts[] = {
        -0.5f,  0.5f,   0.0f, 1.0f,
        -0.5f, -0.5f,   0.0f, 0.0f,
         0.5f, -0.5f,   1.0f, 0.0f,
         0.5f,  0.5f,   1.0f, 1.0f
    };

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // ---------------------------------------------------------
    // SHADERS
    // ---------------------------------------------------------
    unsigned int panelShader = createShader("color.vert", "color.frag");
    unsigned int textureShader = createShader("texture.vert", "texture.frag");
    unsigned int characterShader = createShader("character.vert", "character.frag");


    // ---------------------------------------------------------
    // LOAD TEXTURES NORMAL + SELECTED
    // ---------------------------------------------------------
    unsigned int characterTex = 0;

    const int TEX_COUNT = 12;
    unsigned int textures[TEX_COUNT] = {};
    unsigned int selTextures[TEX_COUNT] = {};


    // normalne
    preprocessTexture(characterTex, "Resources/character.png");
    preprocessTexture(textures[7], "Resources/PR.png");
    preprocessTexture(textures[6], "Resources/SU.png");
    preprocessTexture(textures[4], "Resources/one.png");
    preprocessTexture(textures[5], "Resources/two.png");
    preprocessTexture(textures[2], "Resources/three.png");
    preprocessTexture(textures[3], "Resources/four.png");
    preprocessTexture(textures[0], "Resources/five.png");
    preprocessTexture(textures[1], "Resources/six.png");
    preprocessTexture(textures[8], "Resources/stop.png");
    preprocessTexture(textures[9], "Resources/vent.png");
    preprocessTexture(textures[10], "Resources/open.png");
    preprocessTexture(textures[11], "Resources/close.png");

    // selektovane
    preprocessTexture(selTextures[7], "Resources/PR_sel.png");
    preprocessTexture(selTextures[6], "Resources/SU_sel.png");
    preprocessTexture(selTextures[4], "Resources/one_sel.png");
    preprocessTexture(selTextures[5], "Resources/two_sel.png");
    preprocessTexture(selTextures[2], "Resources/three_sel.png");
    preprocessTexture(selTextures[3], "Resources/four_sel.png");
    preprocessTexture(selTextures[0], "Resources/five_sel.png");
    preprocessTexture(selTextures[1], "Resources/six_sel.png");
    preprocessTexture(selTextures[8], "Resources/stop_sel.png");
    preprocessTexture(selTextures[9], "Resources/vent_sel.png");
    preprocessTexture(selTextures[10], "Resources/open_sel.png");
    preprocessTexture(selTextures[11], "Resources/close_sel.png");

    // ---------------------------------------------------------
    // GRID DIMENSIONS — 6 ROWS × 2 COLS
    // koristimo iste granice kao panel
    // ---------------------------------------------------------
    float panelX = panelLeft;
    float panelY = panelBottom;
    float panelW = panelRight - panelLeft;   // 0.8
    float panelH = panelTop - panelBottom; // 1.8

    int rows = 6;
    int cols = 2;

    float cellW = panelW / cols;
    float cellH = panelH / rows;

    // ---------------------------------------------------------
    // MAIN LOOP
    // ---------------------------------------------------------



// DESNA STRANA (lifit + osoba)
    float rightMinX = -0.1f;   // tu počinje desna polovina
    float rightMaxX = 1.0f;

    // Character pozicija:
    float personX = 0.2f;   // slobodno menjaj kasnije
    float personY = -0.90f; // dno ekrana

    float personW = 0.12f;
    float personH = 0.20f;

    float uX = 0.0f;
    float uY = 0.0f;

    // LIFT DOORS
    float doorY = -0.80f;        // trenutna pozicija gornje ivice vrata (zatvorena)
    float doorTargetY = -0.80f;  // ciljana pozicija vrata

    bool doorOpening = false;
    bool doorClosing = false;

    // index open dugmeta u tvojoj listi je textures[10]
    const int OPEN_BUTTON = 10;


    // ---------------------------------------------------------
    // IZMENJEN LIFT — TAČNO OD 0.85 DO 1.0 PO X, I OD -1.0 DO -0.85 PO Y
    // ---------------------------------------------------------
    float liftX = (0.85f + 1.0f) * 0.5f;   // 0.925
    float liftY = (-1.0f + -0.85f) * 0.5f; // -0.925
    float liftW = (1.0f - 0.85f);          // 0.15
    float liftH = 0.15f;                   // visina 0.15

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, 1);

        // CHARACTER INPUT
        float speed = 0.001f;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            uX -= speed;

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            uX += speed;

        // Ogranicenje kretanja
        float realX = personX + uX;

        // Ako je lik stigao do ivice lifta (0.85)
        if (realX >= 0.85f)
        {
            // Ako je pritisnuto open dugme
            if (selectedIndex == OPEN_BUTTON)
            {
                doorTargetY = -0.60f;   // vrati gore = otvaranje vrata
                doorOpening = true;
                doorClosing = false;
            }
        }
        // Animacija otvaranja vrata lifta
        if (doorOpening)
        {
            if (doorY < doorTargetY)
                doorY += 0.002f;   // brzina otvaranja
            else
                doorOpening = false;
        }


        if (realX < 0.0f) {
            realX = 0.0f;
            uX = realX - personX;
        }

        if (realX > 1.0f) {
            realX = 1.0f;
            uX = realX - personX;
        }


        glClear(GL_COLOR_BUFFER_BIT);

        // -------------------------------- PANEL ------------------------------
        glUseProgram(panelShader);
        glUniform4f(glGetUniformLocation(panelShader, "uColor"),
            0.65f, 0.65f, 0.65f, 1.0f);

        glBindVertexArray(panelVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glUseProgram(panelShader);
        glUniform4f(glGetUniformLocation(panelShader, "uColor"),
            0.0f, 0.0f, 0.0f, 1.0f);

        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, 2);

        // -------------------------------- MOUSE POS --------------------------
        double mouseX, mouseY;
        getMouseNDC(window, mouseX, mouseY);

        // -------------------------------- GRID -------------------------------
        glUseProgram(textureShader);
        glBindVertexArray(quadVAO);
        glUniform1i(glGetUniformLocation(textureShader, "tex"), 0);

        bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        int idx = 0;
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {

                if (idx >= TEX_COUNT) break;

                float cx = panelX + cellW * (c + 0.5f);
                float cy = panelY + panelH - cellH * (r + 0.5f);

                float scaleX = cellW * 0.9f;
                float scaleY = cellH * 0.9f;

                glUniform2f(glGetUniformLocation(textureShader, "uCenter"), cx, cy);
                glUniform2f(glGetUniformLocation(textureShader, "uScale"), scaleX, scaleY);

                glActiveTexture(GL_TEXTURE0);

                if (selectedIndex == idx && selTextures[idx] != 0)
                    glBindTexture(GL_TEXTURE_2D, selTextures[idx]);
                else
                    glBindTexture(GL_TEXTURE_2D, textures[idx]);

                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

                float minX = cx - scaleX * 0.5f;
                float maxX = cx + scaleX * 0.5f;
                float minY = cy - scaleY * 0.5f;
                float maxY = cy + scaleY * 0.5f;

                if (mouseDown && !mouseWasDown)
                {
                    if (mouseX >= minX && mouseX <= maxX &&
                        mouseY >= minY && mouseY <= maxY)
                    {
                        selectedIndex = idx;
                    }
                }
                idx++;
            }
        }

        mouseWasDown = mouseDown;

        // ---------------------------------------------------------
        // RENDER DESNE STRANE: CHARACTER + LIFT
        // ---------------------------------------------------------


        // Character
// Character
        glUseProgram(characterShader);
        glUniform2f(glGetUniformLocation(characterShader, "uCenter"), personX, personY);
        glUniform2f(glGetUniformLocation(characterShader, "uScale"), personW, personH);
        glUniform2f(glGetUniformLocation(characterShader, "uOffset"), uX, uY);
        glUniform1i(glGetUniformLocation(characterShader, "tex"), 0);

        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, characterTex);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // ----- VRATA LIFTA -------

// generišemo dinamične koordinate vrata
       // UPDATE VRATA
        float doorVerts[] = {
            0.85f, doorY,
            0.85f, -0.99f,
            1.0f,  -0.99f,
            1.0f,  doorY
        };

        glBindBuffer(GL_ARRAY_BUFFER, doorVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(doorVerts), doorVerts);

        bool isOpen = doorY > -0.78f; // prag kada smatramo da su vrata "otvorena"
        float cabinShade = isOpen ? 0.45f : 0.15f;

        glUseProgram(panelShader);
        glUniform4f(glGetUniformLocation(panelShader, "uColor"),
            cabinShade, cabinShade, cabinShade, 1.0f);

        glBindVertexArray(liftVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);


        // Lift kabina — VAO sa fiksnim koordinatama

        glUseProgram(panelShader);
        glUniform4f(glGetUniformLocation(panelShader, "uColor"),
            0.15f, 0.15f, 0.15f, 1.0f);   // sivo

        glBindVertexArray(liftVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);



        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}