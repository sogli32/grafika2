#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"
#include <cstdio>
#include <vector>
#include <algorithm>

std::vector<int> floorQueue;   // lista selektovanih spratova (0..7)

const float FLOOR_H = 0.25f; // visina sprata
const int   FLOOR_COUNT = 8;
bool facingLeft = false;

const double TARGET_FPS = 75.0;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

// LOOK queue — oznaka spratova koje treba posetiti
bool floorRequested[FLOOR_COUNT] = { false };

GLFWcursor* ventCursorBlack = nullptr;
GLFWcursor* ventCursorColor = nullptr;
GLFWcursor* defaultCursor = nullptr;

bool ventilationOn = false;
bool cursorLocked = false;


// Smer lifta: 1 nagore, -1 nadole
int liftDirection = 1;
bool extendedOnce = false;


int selectedIndex = -1;
bool mouseWasDown = false; // bolja detekcija klika
bool personInLift = false;
bool personHasEnteredLift = false;
int getNextFloorLOOK(int currentFloor, int& dir)
{
    // 1) pokušaj da nadješ sprat u smeru kretanja
    if (dir == 1)
    {
        for (int f = currentFloor + 1; f < FLOOR_COUNT; f++)
            if (floorRequested[f])
                return f;

        // nema ništa nagore → obrni smer
        dir = -1;

        for (int f = currentFloor - 1; f >= 0; f--)
            if (floorRequested[f])
                return f;
    }
    else // dir == -1
    {
        for (int f = currentFloor - 1; f >= 0; f--)
            if (floorRequested[f])
                return f;

        // nema ništa nadole → obrni smer
        dir = 1;

        for (int f = currentFloor + 1; f < FLOOR_COUNT; f++)
            if (floorRequested[f])
                return f;
    }

    return -1; // ništa nije traženo
}



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
    double frameStart = glfwGetTime();
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

    defaultCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);

    ventCursorBlack = loadImageToCursor("Resources/ventMali.png");
    ventCursorColor = loadImageToCursor("Resources/vent_selMali.png");


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

    // =============================
    // RECTANGLE VAO/VBO (za desnu stranu)
    // =============================
    float rectVerts[] = {
        -0.5f,  0.5f,
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f
    };

    unsigned int rectVAO, rectVBO;
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);

    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVerts), rectVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // =============================
    // VERTIKALNA LINIJA NA SREDINI
    // =============================
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
    // TEXTURED QUAD (za karakter i dugmiće)
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
    // LIFT QUAD (0..1, 0..1) - koristi se sa lift shaderom
    // ---------------------------------------------------------
    float liftQuad[] = {
        0.0f, 1.0f,   // gore-levo
        0.0f, 0.0f,   // dole-levo
        1.0f, 0.0f,   // dole-desno
        1.0f, 1.0f    // gore-desno
    };

    unsigned int liftVAO, liftVBO;
    glGenVertexArrays(1, &liftVAO);
    glGenBuffers(1, &liftVBO);

    glBindVertexArray(liftVAO);
    glBindBuffer(GL_ARRAY_BUFFER, liftVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(liftQuad), liftQuad, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ---------------------------------------------------------
    // DESNI VERTIKALNI PANEL (0.85 → 1.0 po X)
    // ---------------------------------------------------------
    float rightBarVerts[] = {
        0.85f,  1.0f,   // gore-levo
        0.85f, -1.0f,   // dole-levo
        1.0f,  -1.0f,   // dole-desno
        1.0f,   1.0f    // gore-desno
    };

    // ---------------------------------------------------------
    // SHADERS
    // ---------------------------------------------------------
    unsigned int panelShader = createShader("Shaders/color.vert", "Shaders/color.frag");
    unsigned int textureShader = createShader("Shaders/texture.vert", "Shaders/texture.frag");
    unsigned int characterShader = createShader("Shaders/character.vert", "Shaders/character.frag");
    unsigned int liftShader = createShader("Shaders/lift.vert", "Shaders/lift.frag");

    // ---------------------------------------------------------
    // LOAD TEXTURES NORMAL + SELECTED
    // ---------------------------------------------------------
    unsigned int characterTex = 0;
    unsigned int characterLeftTex = 0;
    unsigned int labelTex = 0;

    const int TEX_COUNT = 12;
    unsigned int textures[TEX_COUNT] = {};
    unsigned int selTextures[TEX_COUNT] = {};

    
    preprocessTexture(labelTex, "Resources/label.png");

    // normalne
    preprocessTexture(characterTex, "Resources/character.png");
    preprocessTexture(characterLeftTex, "Resources/character_left.png");

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
    // GRID DIMENSIONS — 6 ROWS × 2 COLS (panel s dugmićima)
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
    // DESNA STRANA (lift + osoba)
    // ---------------------------------------------------------
    float rightMinX = -0.1f;
    float rightMaxX = 1.0f;

    // Character pozicija:
    float personX = 0.2f;
    float personY = -0.63f; // dno ekrana

    float personW = 0.15f;
    float personH = 0.25f;

    float uX = 0.0f;
    float uY = 0.0f;

    // BOJE SPRATOVA
    float colors[8][3] = {
        {0.90f, 0.80f, 0.30f}, // Pikachu žuta (svetla)
        {0.88f, 0.65f, 0.28f}, // toplija žuto-narandžasta
        {0.85f, 0.50f, 0.25f}, // pastel narandžasta
        {0.80f, 0.40f, 0.30f}, // narandžasto-crvena
        {0.75f, 0.30f, 0.30f}, // lagana crvena
        {0.70f, 0.25f, 0.28f}, // dublja crvena
        {0.60f, 0.20f, 0.25f}, // tamna crveno-vinska
        {0.50f, 0.15f, 0.22f}  // tamna bordo (najniži sprat)
    };

    // ---------------------------------------------------------
    // LIFT PARAMETRI (spratovi + kretanje)
    // ---------------------------------------------------------


    int   liftFloor = 2;                     // trenutni sprat (0..7)
    float liftY = -1.0f + liftFloor * FLOOR_H; // donja ivica lifta
    float liftTargetY = liftY;
    bool  liftMoving = false;
    float liftSpeed = 0.001f;                 // TRAŽENA BRZINA

    // VRATA
    float doorY = 0.0f;              // od 0.0 (zatvoreno) do 1.0 (skroz podignuto)
    bool doorOpening = false;
    bool doorClosing = false;

    double doorTimerStart = 0.0;     // vreme kada su se otvorila
    bool doorOpen = false;


    int floorMap[TEX_COUNT];
    for (int i = 0; i < TEX_COUNT; i++)
        floorMap[i] = -1;

    // Tvoje mapiranje spratova:
    floorMap[6] = 1;  // SU.png  → sprat 1
    floorMap[7] = 2;  // PR.png  → sprat 2
    floorMap[4] = 3;  // one.png → sprat 3
    floorMap[5] = 4;  // two.png → sprat 4
    floorMap[2] = 5;  // three.png → sprat 5
    floorMap[3] = 6;  // four.png → sprat 6
    floorMap[0] = 7;  // five.png → sprat 7
    floorMap[1] = 8;  // six.png  → sprat 8

    float prevLiftY = liftY; while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, 1);

        // --------------------------------
        // INPUT – osoba levo/desno
        // --------------------------------
        float speed = 0.001f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            uX -= speed;
            facingLeft = true;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            uX += speed;
            facingLeft = false;
        }


        // --------------------------------
        // 1) LIFT KRETANJE
        // --------------------------------
        if (liftMoving)
        {
            if (fabs(liftY - liftTargetY) < liftSpeed)
            {
                liftY = liftTargetY;
                liftMoving = false;
                // Ako je ventilacija uključena → isključi je kad lift stigne
                if (cursorLocked)
                {
                    ventilationOn = false;
                    cursorLocked = false;

                    glfwSetCursor(window, ventCursorBlack); // vrati crni propeler
                }


                // Stigli → resetuj zahtev za ovaj sprat
                floorRequested[liftFloor] = false;

                // OTVORI VRATA
                doorOpening = true;
                doorClosing = false;
            }
            else
            {
                liftY += (liftY < liftTargetY ? liftSpeed : -liftSpeed);
            }
        }

        // 2) delta lifta za osobu
        float liftDeltaY = liftY - prevLiftY;
        prevLiftY = liftY;

        // 3) ako je osoba u liftu → prati lift
        if (personHasEnteredLift)
            personY += liftDeltaY;

        // 4) izračunaj realnu poziciju osobe
        float realX = personX + uX;
        float realY = personY + uY;

        // 5) detekcija sprata osobe
        int personFloor = (int)((realY + 1.0f) / FLOOR_H);

        if (personFloor < 0)
            personFloor = 0;
        else if (personFloor >= FLOOR_COUNT)
            personFloor = FLOOR_COUNT - 1;


        // 6) da li lift stoji na spratu osobe
        bool liftAtPersonFloor = (!liftMoving && liftFloor == personFloor);

        // 7) osoba ulazi (samo kad su vrata potpuno otvorena)
        bool doorsFullyOpen = (doorY >= 1.0f);

        if (!personHasEnteredLift &&
            liftAtPersonFloor &&
            doorsFullyOpen &&
            realX >= 0.79f)
        {
            personHasEnteredLift = true;
        }

        // 7b) osoba izlazi – samo kad su vrata otvorena 100%
        if (personHasEnteredLift)
        {
            if (doorsFullyOpen && realX < 0.79f)
            {
                personHasEnteredLift = false;
            }
            else if (!doorsFullyOpen && realX < 0.85f)
            {
                uX = 0.85f - personX; // blokiraj izlazak
            }
        }
        // 7c) ograniči osobu da ne ide levo kroz vrata kada nisu potpuno otvorena
        float doorBoundaryX = 0.85f + 0.05f; // 0.05 unutra od vrata = 0.90

        if (personHasEnteredLift && !doorsFullyOpen)
        {
            if (realX < doorBoundaryX)
            {
                uX = doorBoundaryX - personX;  // gurni ga nazad desno
                realX = doorBoundaryX;
            }
        }

        // 8) ograničenje kretanja osobe
        float minX = 0.05f;                             // ✔ pomerena ivica udesno
        float maxX = personHasEnteredLift ? 0.95f : 0.80f;

        if (realX < minX)
            uX = minX - personX;

        if (realX > maxX)
            uX = maxX - personX;

        // recompute
        realX = personX + uX;


        // 9) POZIV LIFTA spolja (C)
        if (!personHasEnteredLift &&
            realX >= 0.80f &&
            glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        {
            floorRequested[personFloor] = true;   // registruj poziv
        }


        // 10) VRATA – animacija
        if (doorOpening)
        {
            doorY += 0.001f;
            if (doorY >= 1.0f)
            {
                doorY = 1.0f;
                doorOpening = false;

                doorOpen = true;
                doorTimerStart = glfwGetTime();  // ✔ TIMER START OVDE
                extendedOnce = false;
            }
        }


        if (doorClosing)
        {
            doorY -= 0.001f;
            if (doorY <= 0.0f)
            {
                doorY = 0.0f;
                doorClosing = false;
                doorOpen = false;
            }
        }

        // automatsko zatvaranje nakon 5s otvorenih vrata
        if (doorOpen && !doorOpening && !doorClosing)
        {
            double elapsed = glfwGetTime() - doorTimerStart;
            if (elapsed >= 5.0)
            {
                doorClosing = true;
            }
        }


        // --------------------------------
        // 11) AKO LIFT STOJI I VRATA SU ZATVORENA → LOOK ALGORITAM
        // --------------------------------
        if (!liftMoving && !doorOpen && !doorOpening && !doorClosing)
        {
            int next = getNextFloorLOOK(liftFloor, liftDirection);
            if (next != -1)
            {
                liftFloor = next;
                liftTargetY = -1.0f + next * FLOOR_H;
                liftMoving = true;
            }
        }

    // --------------------------------
    // RENDER POČINJE OVDE
    // --------------------------------
    glClear(GL_COLOR_BUFFER_BIT);

    // PANEL LEVO
    glUseProgram(panelShader);
    glUniform4f(glGetUniformLocation(panelShader, "uColor"),
        0.65f, 0.65f, 0.65f, 1.0f);

    glBindVertexArray(panelVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // SREDIŠNJA LINIJA
    glUseProgram(panelShader);
    glUniform4f(glGetUniformLocation(panelShader, "uColor"),
        0.0f, 0.0f, 0.0f, 1.0f);

    glBindVertexArray(lineVAO);
    glDrawArrays(GL_LINES, 0, 2);

    // -------------------------------- MOUSE POS --------------------------
    double mouseX, mouseY;
    getMouseNDC(window, mouseX, mouseY);

    // -------------------------------- GRID (dugmići) --------------------
    glUseProgram(textureShader);
    glBindVertexArray(quadVAO);
    glUniform1i(glGetUniformLocation(textureShader, "tex"), 0);

    bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    int idx = 0;
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            if (idx >= TEX_COUNT) break;

            float cx = panelX + cellW * (c + 0.5f);
            float cy = panelY + panelH - cellH * (r + 0.5f);

            float scaleX = cellW * 0.9f;
            float scaleY = cellH * 0.9f;

            glUniform2f(glGetUniformLocation(textureShader, "uCenter"), cx, cy);
            glUniform2f(glGetUniformLocation(textureShader, "uScale"), scaleX, scaleY);

            glActiveTexture(GL_TEXTURE0);

            int f = floorMap[idx];
            if (f != -1 && floorRequested[f - 1])
                glBindTexture(GL_TEXTURE_2D, selTextures[idx]);  // dugme svetli jer je traženo
            else
                glBindTexture(GL_TEXTURE_2D, textures[idx]);


            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            float minX = cx - scaleX * 0.5f;
            float maxX = cx + scaleX * 0.5f;
            float minY = cy - scaleY * 0.5f;
            float maxY = cy + scaleY * 0.5f;

            if (mouseDown && !mouseWasDown)
            {
                if (personHasEnteredLift)
                {
                    // kliknut baš ovaj pravougaonik?
                    bool inside =
                        (mouseX >= minX && mouseX <= maxX &&
                            mouseY >= minY && mouseY <= maxY);

                    if (inside)
                    {
                        // SPRATOVI (dugmići 0..7)
                        int f = floorMap[idx];
                        if (f != -1)
                        {
                            floorRequested[f - 1] = true;
                        }

                        // VENT dugme = idx 9
                        if (idx == 9 && personHasEnteredLift)
                        {
                            ventilationOn = true;
                            cursorLocked = true;
                        }


                        // OPEN dugme = idx 10
                        if (idx == 10)
                        {
                            // vrata moraju biti potpuno otvorena i mirna
                            if (doorOpen && !doorOpening && !doorClosing && !extendedOnce)
                            {
                                doorTimerStart = glfwGetTime(); // produži još 5 sekundi
                                extendedOnce = true;
                            }
                        }

                        // CLOSE dugme = idx 11
                        if (idx == 11)
                        {
                            if (doorOpen && !doorOpening && !doorClosing)
                            {
                                doorClosing = true;
                                // možeš da ostaviš doorOpen = true dok se ne zatvore,
                                // ali ako ti je lakše za logiku, ostavi kao do sada:
                                // doorOpen = false;
                            }
                        }
                    }
                }
            }


            idx++;
        }
    }

    mouseWasDown = mouseDown;

    // ---------------------------------------------------------
    // RENDER DESNE STRANE
    // ---------------------------------------------------------
    glUseProgram(panelShader);
    glBindVertexArray(rectVAO);

    float slotH = 0.25f;

    for (int i = 0; i < 8; i++)
    {
        float y1 = -1.0f + slotH * i;
        float y2 = y1 + slotH;

        float verts[] = {
            0.0f, y1,
            0.0f, y2,
            0.85f, y2,
            0.85f, y1
        };

        glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

        glUniform4f(glGetUniformLocation(panelShader, "uColor"),
            colors[i][0], colors[i][1], colors[i][2], 1.0f);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }



    // DESNI PANEL
    glUseProgram(panelShader);
    glUniform4f(glGetUniformLocation(panelShader, "uColor"),
        0.3f, 0.3f, 0.3f, 1.0f);

    glBindVertexArray(rectVAO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(rightBarVerts), rightBarVerts);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // ---------------------------------------------------------
    // LIFT
    // ---------------------------------------------------------
    glUseProgram(liftShader);

    glUniform1f(glGetUniformLocation(liftShader, "uLeftX"), 0.85f);
    glUniform1f(glGetUniformLocation(liftShader, "uRightX"), 1.0f);
    glUniform1f(glGetUniformLocation(liftShader, "uBottomY"), liftY);
    glUniform1f(glGetUniformLocation(liftShader, "uTopY"), liftY + FLOOR_H);

    glUniform4f(glGetUniformLocation(liftShader, "uLiftColor"),
        0.85f, 0.85f, 0.85f, 1.0f);

    glBindVertexArray(liftVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // ----- VRATA -----
    glUseProgram(panelShader);

    float doorLeft = 0.85f;
    float doorRight = 0.87f;
    float doorBottom = liftY;
    float doorTop = liftY + FLOOR_H;

    float currentTop = doorBottom + (doorTop - doorBottom) * (1.0f - doorY);

    float doorVerts[] = {
        doorLeft,  currentTop,
        doorLeft,  doorBottom,
        doorRight, doorBottom,
        doorRight, currentTop
    };

    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(doorVerts), doorVerts);

    glUniform4f(glGetUniformLocation(panelShader, "uColor"),
        0.2f, 0.2f, 0.2f, 1.0f);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // ---------------------------------------------------------
    // CHARACTER
    // ---------------------------------------------------------
    glUseProgram(characterShader);

    // (uLiftOffsetY ti sada realno ne treba, jer menjamo personY,
    // ali ako ga koristiš, stavi 0)
    glUniform1f(glGetUniformLocation(characterShader, "uLiftOffsetY"), 0.0f);

    glUniform2f(glGetUniformLocation(characterShader, "uCenter"), personX, personY);
    glUniform2f(glGetUniformLocation(characterShader, "uScale"), personW, personH);
    glUniform2f(glGetUniformLocation(characterShader, "uOffset"), uX, uY);
    glUniform1i(glGetUniformLocation(characterShader, "tex"), 0);

    glBindVertexArray(quadVAO);
    glActiveTexture(GL_TEXTURE0);
    if (facingLeft)
        glBindTexture(GL_TEXTURE_2D, characterLeftTex);
    else
        glBindTexture(GL_TEXTURE_2D, characterTex);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    if (cursorLocked && ventilationOn && ventCursorColor)
    {
        glfwSetCursor(window, ventCursorColor);
    }
    else
    {
        glfwSetCursor(window, ventCursorBlack); // ← Umesto ventCursorBlack
    }

    glUseProgram(textureShader);
    glBindVertexArray(quadVAO);
    glUniform1i(glGetUniformLocation(textureShader, "tex"), 0);

    // recimo u gornji levi ugao
    float labelCenterX = -0.85f;   // pomeri po želji
    float labelCenterY = -0.95f;
    float labelW = 0.3f;           // širina pravougaonika
    float labelH = 0.1f;          // visina

    glUniform2f(glGetUniformLocation(textureShader, "uCenter"), labelCenterX, labelCenterY);
    glUniform2f(glGetUniformLocation(textureShader, "uScale"), labelW, labelH);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, labelTex);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);



    glfwSwapBuffers(window);
    glfwPollEvents();

    double frameEnd = glfwGetTime();
    double frameTime = frameEnd - frameStart;

    if (frameTime < TARGET_FRAME_TIME)
    {
        // prosta busy-wait varijanta (dovoljna za projekat)
        while (glfwGetTime() - frameStart < TARGET_FRAME_TIME) {}
    }
}

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
