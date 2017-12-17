//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
// INF01047 Fundamentos de Computação Gráfica 2017/2
//               Prof. Eduardo Gastal
//
//                  Trabalho Final
//
//     The Insane Game
//
//     Andy Ruiz Garramones - 00274705
//     Guilherme Gomes Haetinger - 00274702
//     Lucas Nunes Alegre - 00274693
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <time.h>
#include <random>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Bibliotecas audio
#include <irrKlang.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "arrow.h"
#include "enemy.h"
#include "collision.hpp"

// === DEFINES ===
#define DIED camera_position_c.y < -0.5f
#define INITIAL_THETA -0.5f
#define INITIAL_PHI -0.465
#define ALTURAHERO 3.0f
#define START_POSITION glm::vec4(0.0f ,ALTURAHERO + 1.0f, 0.0f, 1.0f)
#define MAX_THETA_MENU 0.7
#define MIN_THETA_MENU -0.7
#define MAX_PHI_MENU 0.5
#define MIN_PHI_MENU -0.5

#define SPHERE 0
#define BUNNY  1
#define PLANE  2
#define AIM  3
#define ARM 4
#define BOW 5
#define ARROW 6
#define ARROWT 7
#define ARROWP 8
#define CUBE 9
#define CUBE1 10
#define CUBE2 11
#define CUBE3 12
#define COW 13
#define WALLPAPER 14
#define MOON 15
#define GHOST1 16
#define GHOST2 17
#define GHOST3 18
#define GHOST4 19

using namespace std;
// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    void*        first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    int          num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

///Struct para as plataformas contendo os pontos xyz e escalacao de cada Cubo
struct Cubo
{
    float x;
    float y;
    float z;
    float dx;
    float dy;
    float dz;
    int textureCode;

    Cubo(float x1, float y1, float z1, float dx1, float dy1, float dz1, int code)
    {
        x = x1;
        y = y1;
        z = z1;
        dx = dx1;
        dy = dy1;
        dz = dz1;
        textureCode = code;
    }
};

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging
void playGame();
int menu();
void optionsMenu();

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

///Element drawing
void drawBackground(glm::vec4 &wallpaperPos);
void createEnemies(int numEnemies, std::vector<Enemy>* enemies, std::vector<Cubo> cubos);
void generateFinalPlataform(vector<Cubo> *cubos, Cubo last);

bool entreLimites(float posCamera, float eixo, float delta,float erro);
bool caiuDemais(float antigoEixo,float novoEixo, float posEixo, float delta, float erro);
void processaWASD(float *novoX,float antigoX,float *novoZ,float antigoZ,float deslocamento,glm::vec4 u,glm::vec4 w);
void atualizaPulo();
void testaChao(float novoX,float novoZ,unsigned int *startFall,unsigned int actualSecond,std::vector<Cubo> cubos);
void trataColisaoCubo(float novoX,float antigoY,float novoZ, int nearestCube,bool *invadiuObjeto,bool *pousou,std::vector<Cubo> cubos);
bool processaPouso(float antigoY,float cuboY,float cuboDY,float correcao);
void processaMovimentos(bool WASD,float antigoX,float *novoX,float antigoZ,float *novoZ,float antigoY,std::vector<Cubo> &cubos, glm::vec4 teleportPos);
void loadLevelPlatforms(std::vector<Cubo> &cubos);
void aplicaGravidade();
void processaColisao(float novoX,float antigoY,float novoZ,bool *invadiuObjeto,std::vector<Cubo> cubos);
void testCheckPoint(int nearestCube,std::vector<Cubo> cubos);
int getNearestCube(float novoX,float antigoY,float novoZ,std::vector<Cubo> cubos);
void resetLife(float *novoX,float *novoZ,std::vector<Cubo> &cubos);
ArrowType selectArrowType();
void handleTeleport(float * novoX,float * novoZ,std::vector<Cubo> cubos, glm::vec4 teleportPos);
void loadFirstMap(std::vector<Cubo> &cubos);
void loadSecondMap(std::vector<Cubo> &cubos);
void loadThirdMap(std::vector<Cubo> &cubos);
void loadFourthMap(std::vector<Cubo> &cubos);
int getLevelEnemies(int waveCounter);
void printDialog(std::vector<std::string> dialog, double time, double startTime);


// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

int g_width, g_height;

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;


//===============================================================================================================
///Variaveis Globais
bool pressW = false;
bool pressS = false;
bool pressA = false;
bool pressD = false;
bool pressT = false;
bool pressR = false;
bool pressE = false;
bool pressSpace = false;
bool JUMPING = false;
bool DOUBLEJUMPING = false;
bool MAGICPLATFORM = false;
bool CAINDO = false;
bool TELEPORTED = false;
bool arrowReplaced = true;
bool charging = false;
bool enterPressed = false;
bool busyWKey = false;
bool busySKey = false;
bool busyJUMPKey = false;
bool endingPlatform = false;
bool WASD = false;
float gravidade = 0.005;
float antigoY = 0;
float antigoX = 0;
float antigoZ = 0;
float novoX;
float novoZ;
float deslocamento = 1.5f;
int oldCubo = 0;
int startJump = 0;
double chargeTime = 0.0f;
double actualSecond;
double whileTime;
unsigned int startFall = 0;

int level = 1;

glm::vec4 nextPosition;
glm::vec4 camera_view_vector;

glm::vec4 w;
glm::vec4 u;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo par// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origema botão do meio do mouse

glm::vec4 camera_position_c = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "c", centro da câmera
glm::vec4 camera_up_vector  = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up"

using namespace irrklang;
ISoundEngine* engine;
GLFWwindow* window;

glm::mat4 model;

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    window = glfwCreateWindow(mode->width, mode->height, "The Insane Game", NULL, NULL);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, mode->width, mode->height); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    engine = createIrrKlangDevice();
    if (!engine)
        return 0; // error starting up the engine

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slide 217 e 219 do documento no Moodle
    // "Aula_03_Rendering_Pipeline_Grafico.pdf".
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/tc-earth_daymap_surface.jpg");      // TextureImage0
    LoadTextureImage("../../data/tc-earth_nightmap_citylights.gif"); // TextureImage1
    LoadTextureImage("../../data/skin.jpg");      // TextureImage2
    LoadTextureImage("../../data/bowText.jpg");      // TextureImage3
    LoadTextureImage("../../data/moon.jpg");      // TextureImage4
    LoadTextureImage("../../data/cube/hazard.jpg");      // TextureImage5
    LoadTextureImage("../../data/cube/bloc.jpg");      // TextureImage6
    LoadTextureImage("../../data/cube/box2.jpg");      // TextureImage7
    LoadTextureImage("../../data/ghost/normalMap.png");      // TextureImage8
    LoadTextureImage("../../data/space.jpg");      // TextureImage9
    LoadTextureImage("../../data/ghost/ghostText.png");      // TextureImage10
    LoadTextureImage("../../data/ghost/ghostTextBlue.jpg");      // TextureImage11
    LoadTextureImage("../../data/ghost/ghostTextBlack.jpg");      // TextureImage12
    LoadTextureImage("../../data/ghost/ghostTextSaturated.jpg");      // TextureImage13
    LoadTextureImage("../../data/cube/oie.png");      // TextureImage14



    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel bunnymodel("../../data/bunny.obj");
    ComputeNormals(&bunnymodel);
    BuildTrianglesAndAddToVirtualScene(&bunnymodel);

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel cowmodel("../../data/cow.obj");
    ComputeNormals(&cowmodel);
    BuildTrianglesAndAddToVirtualScene(&cowmodel);

    ObjModel inkymodel("../../data/inky/Inky.obj");
    ComputeNormals(&inkymodel);
    BuildTrianglesAndAddToVirtualScene(&inkymodel);

    ObjModel clydemodel("../../data/clyde/Clyde.obj");
    ComputeNormals(&clydemodel);
    BuildTrianglesAndAddToVirtualScene(&clydemodel);

    ObjModel pinkymodel("../../data/pinky/Pinky.obj");
    ComputeNormals(&pinkymodel);
    BuildTrianglesAndAddToVirtualScene(&pinkymodel);

    ObjModel righthandmodel("../../data/stretch.obj");
    ComputeNormals(&righthandmodel);
    BuildTrianglesAndAddToVirtualScene(&righthandmodel);

    ObjModel cubemodel("../../data/cube.obj");
    ComputeNormals(&cubemodel);
    BuildTrianglesAndAddToVirtualScene(&cubemodel);

    ObjModel ghostmodel("../../data/ghost/ghostOBJ.obj");
    ComputeNormals(&ghostmodel);
    BuildTrianglesAndAddToVirtualScene(&ghostmodel);

    ///Carregando objetos da animacao
    for(int i = 1; i<=9; i++)
    {
        std::stringstream sst;
        sst << i;
        std::string st = "../../data/bow_anim/bow_stretch_00000" + sst.str() + ".obj";
        char* fileDir = new char[st.length()];
        strcpy(fileDir, st.c_str());
        ObjModel bowmodel(fileDir);
        ComputeNormals(&bowmodel);
        BuildTrianglesAndAddToVirtualScene(&bowmodel);
    }
     ///Carregando objetos da animacao
    for(int i = 10; i<=20; i++)
    {
        std::stringstream sst;
        sst << i;
        std::string st = "../../data/bow_anim/bow_stretch_0000" + sst.str() + ".obj";
        char* fileDir = new char[st.length()];
        strcpy(fileDir, st.c_str());
        ObjModel bowmodel(fileDir);
        ComputeNormals(&bowmodel);
        BuildTrianglesAndAddToVirtualScene(&bowmodel);
    }

    ObjModel lefthandmodel("../../data/hand.obj");
    ComputeNormals(&lefthandmodel);
    BuildTrianglesAndAddToVirtualScene(&lefthandmodel);


    ObjModel arrowmodel("../../data/Arrow.obj");
    ComputeNormals(&arrowmodel);
    BuildTrianglesAndAddToVirtualScene(&arrowmodel);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 66 à 68 do documento "Aula_13_Clipping_and_Culling.pdf".
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 22 à 34 do documento "Aula_13_Clipping_and_Culling.pdf".
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Variáveis auxiliares utilizadas para chamada à função
    // TextRendering_ShowModelViewProjection(), armazenando matrizes 4x4.
    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;


    while(true)
    {
        ///Enquanto no menu toca a musica do menu
        engine->play2D("../../audio/menuSong.wav", true);
        int opMenu = menu();
        switch(opMenu)
        {

        case 0 :
            ///Quando entrar no jogo pausa a musica do menu
            engine->stopAllSounds();
            engine->play2D("../../audio/initGame.wav", false);
            playGame();
            break;

        case 1:
            optionsMenu();
            break;

        case 2:
            engine->drop();
            return 0;
            break;

        default:
            break;

        }
        engine->stopAllSounds();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

///Printa os dialogos a partir do tempo e a string atual
void printDialog(vector<std::string> dialog, double time, double startTime)
{

    if(glfwGetTime() - startTime < time && glfwGetTime() - startTime > 0)
    {
        int counter = 0;
        for(unsigned i = 0; i < dialog.size(); i++)
        {
            TextRendering_PrintString(window, dialog[i], 0.0f, 0.5f - counter*0.075f, 3.0f);
            counter++;
        }
    }

}

///Gera inimigos randomicamente em volta dos cubos da fase atual
void createEnemies(int numEnemies, std::vector<Enemy>* enemies, std::vector<Cubo> cubos)
{

    srand(time(NULL));
    int maxIndex = cubos.size() - 1;
    int randomicCube;
    float randomx;
    float diffx;
    float rx;
    float randomz;
    float diffz;
    float rz;

    for(int i = 0; i < numEnemies; i++)
    {
        randomicCube = rand() % maxIndex + 1;
        randomx = ((float) rand()) / (float) RAND_MAX;
        diffx = 100.0f;
        rx = randomx * diffx;
        if(i%2 == 0)
        {
            rx = -rx;
        }
        randomz = ((float) rand()) / (float) RAND_MAX;
        diffz = 100.0f;
        rz = randomz * diffz;
        if(i%4 == 0)
        {
            rz = -rz;
        }
        enemies->push_back(Enemy(glm::vec4(cubos[randomicCube].x + rx, cubos[randomicCube].y + 30.0f, cubos[randomicCube].z + rz, 1.0f), false, "ghost"));
    }
}

///Gera a plataforma do fim do jogo
void generateFinalPlataform(vector<Cubo> *cubos, Cubo last)
{

    cubos->push_back(Cubo(last.x, last.y, last.z + last.dz/2 + 75.0f,100.0f, 1.0f, 100.0f, 1));

}
///Mostra o menu e interage com teclas pressionadas pelo usuario
int menu()
{
    int selectionTime = 0;

    float startPos = -0.5f;
    float startSize = 2.5f;
    float optionPos = -0.65f;
    float optionSize = 1.0f;
    float exitPos = -0.8f;
    float exitSize = 1.0f;

    int selectPos = 0;

    while(!enterPressed)
    {
        selectionTime++;
        if(selectionTime == 5)
        {
            selectionTime = 0;
        }

        if(selectionTime == 0)
        {
            if(pressW && !busyWKey && selectPos > 0)
            {
                busyWKey = true;
                selectPos--;
            }
            if(pressS && !busySKey && selectPos < 2)
            {
                busySKey = true;
                selectPos++;
            }

            switch(selectPos)
            {
            case 0:
                startSize = 2.5f;
                optionSize = 1.0f;
                exitSize = 1.0f;
                break;
            case 1:
                startSize = 1.0f;
                optionSize = 2.5f;
                exitSize = 1.0f;
                break;
            case 2:
                startSize = 1.0f;
                optionSize = 1.0f;
                exitSize = 2.5f;
                break;
            default:
                break;
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_id);
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slide 165 do documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::vec4 camera_position_c  = glm::vec4(x,y,z,1.0f); // Ponto "c", centro da câmera
        glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slide 169 do
        // documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 198-200 do documento
        // "Aula_09_Projecoes.pdf".
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -100.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slide 234 do
            // documento "Aula_09_Projecoes.pdf".
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // veja slide 243 do documento "Aula_09_Projecoes.pdf".
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem
        // model = Matrix_Rotate_Y((float)glfwGetTime() * 0.1f);
        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, COW);
        DrawVirtualObject("cow");

        TextRendering_PrintString(window, "Start", 0.0f, startPos, startSize);
        TextRendering_PrintString(window, "Options", 0.0f, optionPos, optionSize);
        TextRendering_PrintString(window, "exit", 0.0f, exitPos, exitSize);

        glfwSwapBuffers(window);


        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    enterPressed = false;
    return selectPos;

}

///Mostra as opcoes do jogo
void optionsMenu()
{

    while(!enterPressed){

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_id);
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slide 165 do documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::vec4 camera_position_c  = glm::vec4(x,y,z,1.0f); // Ponto "c", centro da câmera
        glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slide 169 do
        // documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 198-200 do documento
        // "Aula_09_Projecoes.pdf".
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -100.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slide 234 do
            // documento "Aula_09_Projecoes.pdf".
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // veja slide 243 do documento "Aula_09_Projecoes.pdf".
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        TextRendering_PrintString(window, "Sometimes life gives you no options", -1.0f, 0.0f, 4.0f);

        glfwSwapBuffers(window);


        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    enterPressed = false;


}

///Coloca pra jogar
void playGame()
{

    double dialog1Start = glfwGetTime();
    double tutorialBegin = dialog1Start + 15;
    double endBegin1 = 0;
    double endBegin2 = 0;
    int waveCounter = -1;
    int waveCounterLimit = 4;
    bool reachedAnEnd = false;
    int stretchCount = 0;
    double stretchController = glfwGetTime();
    double arrowRateController = 0;
    enterPressed = false;
    std::vector<Cubo> cubos;
    loadFirstMap(cubos);


    std::vector<Arrow> arrows;
    std::vector<Enemy> enemies;

    int cubeText = 0;
    int enemyText = 0;

    resetLife(&novoX, &novoZ, cubos);

    ArrowType arrowType = normal;
    glm::vec4 teleportPosition;
    glm::vec4 *plataformArrowPosition = nullptr;

    engine->play2D("../../audio/song1.wav", true);

    model = Matrix_Identity(); // Transformação identidade de modelagem

    ///Carrega strings de dialogos
    std::vector<std::string> beginDialog;
    beginDialog.push_back("Come find me...");
    beginDialog.push_back("Release the ultimate power...");

    std::vector<std::string> tutorialDialog;
    tutorialDialog.push_back("Press WASD to walk");
    tutorialDialog.push_back("Press SPACE to jump(2x quickly to jump higher)");
    tutorialDialog.push_back("E - NORMAL ARROW");
    tutorialDialog.push_back("R - PLATAFORM CREATION ARROW");
    tutorialDialog.push_back("T - TELEPORT ARROW");


    std::vector<std::string> endDialog1;
    endDialog1.push_back("Thank you for releasing me...");
    endDialog1.push_back("For you have been my only hope");
    endDialog1.push_back("fearless human,");
    endDialog1.push_back("I will give you knowledge!");
    std::vector<std::string> endDialog2;
    endDialog2.push_back("The earth is not round as you foolish");
    endDialog2.push_back("beigns think it is...");
    endDialog2.push_back("The earth is actually");
    endDialog2.push_back("A full set of triangle meshes that");
    endDialog2.push_back(" simulates an elipsoid...");
    endDialog2.push_back("Now may us part ways");
    endDialog2.push_back("and never see each other again...");



    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {

        if(glfwGetTime() - stretchController > 0.02 && stretchCount+1 < 20 && charging)
        {

            stretchCount++;
            stretchController = glfwGetTime();
        }

        actualSecond = (glfwGetTime() * 10);

        // Aqui executamos as operações de renderização

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).

        glUseProgram(program_id);
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        ///Vetores w u para camera + movimentacao
        glm::vec4 camera_view_vector = glm::vec4(x,y,z,0.0f); // Vetor "view", sentido para onde a câmera está virada
        camera_view_vector = camera_view_vector/norm(camera_view_vector);
        w = -camera_view_vector;
        u = crossproduct(camera_up_vector, w);
        w = w/norm(w);
        u = u/norm(u);
        glm::vec4 v = crossproduct(w,u);

        ///Caso troque de fase
        if(endingPlatform){
            cubos.clear();
            loadLevelPlatforms(cubos);
            novoX = camera_position_c.x;
            novoZ = camera_position_c.z;
            endingPlatform = false;
            CAINDO = true;
            engine->play2D("../../audio/teleport.wav", false);
        }

        ///Preparar variaveis para tratamento de colisao
        antigoX = camera_position_c.x;
        antigoZ = camera_position_c.z;
        WASD = pressA || pressD || pressS || pressW;


        //Desenha wallpaper
        glDepthMask(false);
        glm::vec4 wallpaperPos = camera_position_c;
        drawBackground(wallpaperPos);


        // Desenhamos o modelo da esfera
        model = Matrix_Translate(wallpaperPos.x - 300.0f, wallpaperPos.y + 10.0f,wallpaperPos.z + 100.0f)
                * Matrix_Scale(100,100,100)
                * Matrix_Rotate_Z(0.6f)
                * Matrix_Rotate_X(0.2f)
                * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.01f);

        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, SPHERE);
        DrawVirtualObject("sphere");

        glDepthMask(true);

        model = Matrix_Translate(0,-10,0) * Matrix_Scale(1000,0,1000);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, MOON);
        DrawVirtualObject("plane");

        ///Processamento das acoes
        processaWASD(&novoX, antigoX, &novoZ, antigoZ, deslocamento, u, w);
        if(pressSpace)
        {
            atualizaPulo();
        }
        processaMovimentos(WASD, antigoX, &novoX, antigoZ, &novoZ, antigoY, cubos, teleportPosition);

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slide 169 do
        // documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 198-200 do documento
        // "Aula_09_Projecoes.pdf".
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -500.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slide 234 do
            // documento "Aula_09_Projecoes.pdf".
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // veja slide 243 do documento "Aula_09_Projecoes.pdf".
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }
        if(DIED && !enterPressed)
        {
            waveCounter = -1;
            enemies.clear();
            reachedAnEnd = false;
            TextRendering_PrintString(window, "YOU DIED", -0.5f, 0.0f, 5.0f);
            TextRendering_PrintString(window, "Press enter to continue...", -0.5f, -0.5f, 1.0f);
        }
        else
        {

            // Enviamos as matrizes "view" e "projection" para a placa de vídeo
            // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
            // efetivamente aplicadas em todos os pontos.
            glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

            std::stringstream sst;
            sst << (int) (stretchCount)+1;
            std::string st = "bow_anim" + sst.str();
            char* bowStretch = new char[st.length()];
            strcpy(bowStretch, st.c_str());
            glm::vec4 bowPos = 1.5f*camera_view_vector + 0.15f*u + -0.2f*v;
            model = Matrix_Translate(camera_position_c[0] + bowPos[0],
                                     camera_position_c[1] + bowPos[1],
                                     camera_position_c[2] + bowPos[2])
                    * Matrix_Rotate(3.14/3 - ((float)stretchCount/(19))*(3.14/3), camera_view_vector)
                    * Matrix_Rotate(g_CameraPhi, u)
                    * Matrix_Rotate(3.14/2 + g_CameraTheta, camera_up_vector)
                    * Matrix_Scale(0.065, 0.035, 0.065);
            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(object_id_uniform, BOW);
            DrawVirtualObject(bowStretch);


            glm::vec4 handPos =   ((float)stretchCount/(19)) * 1.5f*camera_view_vector + (1 -((float)stretchCount/(19))) * 1.45f*camera_view_vector
                                  + ((float)stretchCount/(19)) * 0.05f*u + (1 -((float)stretchCount/(19))) * -0.3f*u
                                  + ((float)stretchCount/(19)) * -0.55f*v + (1 -((float)stretchCount/(19))) * -0.3f*v;
            model =  Matrix_Translate(camera_position_c[0] + handPos[0],
                                      camera_position_c[1] + handPos[1],
                                      camera_position_c[2] + handPos[2])
                     * Matrix_Rotate(-3.14/6- ((float)stretchCount/(19))*(3.14/3), camera_view_vector)
                     * Matrix_Rotate(g_CameraPhi, u)
                     * Matrix_Rotate(g_CameraTheta + 3.14, camera_up_vector)
                     * Matrix_Scale(0.009, 0.009, 0.009);
            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(object_id_uniform, ARM);
            DrawVirtualObject("hand");

            glm::vec4 righthandPos =   ((float)stretchCount/(19)) * 0.15f*camera_view_vector + (1 -((float)stretchCount/(19))) * 1.0f*camera_view_vector
                                       + ((float)stretchCount/(19)) * 0.1f*u + (1 -((float)stretchCount/(19))) * 0.1f*u
                                       + ((float)stretchCount/(19)) * -0.25f*v + (1 -((float)stretchCount/(19))) * -0.25f*v;


            model =  Matrix_Translate(camera_position_c[0] + righthandPos[0],
                                      camera_position_c[1] + righthandPos[1],
                                      camera_position_c[2] + righthandPos[2])
                     * Matrix_Rotate(3.14 - 3.14/5 - ((float)stretchCount/(19))*(5*3.14/16), camera_view_vector)
                     * Matrix_Rotate(g_CameraPhi, u)
                     * Matrix_Rotate(g_CameraTheta + 3.14, camera_up_vector)
                     * Matrix_Scale(0.09, 0.11, 0.1);
            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(object_id_uniform, ARM);
            DrawVirtualObject("rightHand");

            model = Matrix_Translate(camera_position_c[0] + camera_view_vector[0],
                                     camera_position_c[1] + camera_view_vector[1],
                                     camera_position_c[2] + camera_view_vector[2])
                    * Matrix_Rotate(g_CameraPhi + 3.14/2, u)
                    * Matrix_Rotate(3.14/2 + g_CameraTheta, camera_up_vector)
                    *Matrix_Scale(0.0075, 1.5, 0.0075);
            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(object_id_uniform, AIM);
            DrawVirtualObject("plane");

            for(unsigned i = 0; i < cubos.size(); i++)
            {
                model = Matrix_Translate(cubos[i].x,cubos[i].y,cubos[i].z)
                        * Matrix_Scale(cubos[i].dx ,cubos[i].dy,cubos[i].dz );
                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                glUniform1i(object_id_uniform, cubos[i].textureCode);
                DrawVirtualObject("cube");
            }

            ///Enquanto nao se acabar o jogo
            if(!reachedAnEnd)
            {

                ///Caso se chege na ultima fase e se acabem todas as waves se acaba o jogo
                if(waveCounter == waveCounterLimit && level == 4 )
                {
                    reachedAnEnd = true;
                    waveCounter = waveCounterLimit;
                    generateFinalPlataform(&cubos, cubos[cubos.size()-1]);

                }
                else
                {
                    ///geracao de inimigos
                    if(enemies.size() == 0)
                    {
                        ///Teste se mudou de fase
                        if(endingPlatform && waveCounter>=0)
                        {
                            endingPlatform = false;
                        }

                        createEnemies(getLevelEnemies(waveCounter), &enemies,cubos);
                        waveCounter++;
                        enemyText++;
                        cubeText++;
                        if(enemyText > 3)
                        {
                            enemyText = 0;
                        }
                        if(cubeText > 2)
                        {
                            cubeText = 0;
                        }
                    }
                }
            }
            //Desenha o final
            else
            {
                enemies.clear();
                model = Matrix_Translate(cubos[cubos.size()-1].x, cubos[cubos.size()-1].y + cubos[cubos.size()-1].dy + 5, cubos[cubos.size()-1].z)
                        *Matrix_Rotate_Y(3.14/2)
                        *Matrix_Scale(10, 10, 10);

                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                glUniform1i(object_id_uniform, COW);
                DrawVirtualObject("cow");
            }
            //Draw and update enemies
            for(unsigned i = 0; i < enemies.size(); i++)
            {

                updateEnemy(&enemies[i], camera_position_c, whileTime);

                model = Matrix_Translate(enemies[i].pos.x, enemies[i].pos.y, enemies[i].pos.z)
                        * Matrix_Scale(enemies[i].scale.x, enemies[i].scale.y + enemies[i].Y_deviation, enemies[i].scale.z)
                        * Matrix_Rotate_Y(enemies[i].rotation_Y);
                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                glUniform1i(object_id_uniform, enemyText + 16);
                DrawVirtualObject(enemies[i].name.c_str());
            }

            if(pressE || pressR || pressT)
                arrowType = selectArrowType();

            // Create plataform
            if(g_RightMouseButtonPressed && plataformArrowPosition != nullptr)
            {
                cubos[0].x = plataformArrowPosition->x;
                cubos[0].y = plataformArrowPosition->y;
                cubos[0].z = plataformArrowPosition->z;
                MAGICPLATFORM = true;
            }

            if(g_LeftMouseButtonPressed && !charging && arrowReplaced)
            {
                charging = true;
                chargeTime = (glfwGetTime() * 10);
            }
            // Shoot arrow
            if(!g_LeftMouseButtonPressed && charging && arrowReplaced)
            {
                arrowReplaced = false;
                arrowRateController = glfwGetTime();
                stretchCount = 0;
                charging = false;
                chargeTime = (glfwGetTime() * 10) - chargeTime;
                if(chargeTime > 10.0)
                    chargeTime = 10.0;
                arrows.push_back(Arrow(camera_position_c, (float)chargeTime*camera_view_vector, g_CameraTheta, g_CameraPhi));
                if(arrowType == teleport)
                {
                    arrows[arrows.size()-1].type = teleport;
                }
                else if(arrowType == plataform)
                {
                    arrows[arrows.size()-1].type = plataform;
                    plataformArrowPosition = &(arrows[arrows.size()-1].pos);
                }

                engine->play2D("../../audio/arco.mp3", false);
            }
            if(glfwGetTime() - arrowRateController > 20)
            {
                arrowReplaced = true;
                glm::vec4 arrowPos =  ((float)stretchCount/(19)) * 1.0f*camera_view_vector + (1 -((float)stretchCount/(19))) * 2.0f*camera_view_vector
                                      + ((float)stretchCount/(19)) * 0.15f*u + (1 -((float)stretchCount/(19))) * 0.06f*u
                                      + ((float)stretchCount/(19)) * -0.25f*v + (1 -((float)stretchCount/(19))) * -0.15f*v;

                model = Matrix_Translate(camera_position_c[0] + arrowPos[0],
                                         camera_position_c[1] + arrowPos[1],
                                         camera_position_c[2] + arrowPos[2])
                        * Matrix_Rotate(3.14/4, camera_view_vector)
                        * Matrix_Rotate(g_CameraPhi, u)
                        * Matrix_Rotate( g_CameraTheta + 3.14/24, camera_up_vector)
                        * Matrix_Scale(1, 1, 1);
                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                if(arrowType == teleport)
                    glUniform1i(object_id_uniform, ARROWT);
                else if(arrowType == plataform)
                    glUniform1i(object_id_uniform, ARROWP);
                else
                    glUniform1i(object_id_uniform, ARROW);
                DrawVirtualObject("arrow");
            }

            bool arrowCollides = false;
            ///Tratamento de arrows
            for(unsigned i = 0; i < arrows.size(); i++)
            {
                updateArrow(&arrows[i], whileTime);

                // Colisao com cubos
                for(int j = 0; j < (int) cubos.size(); j++)
                {
                    model = Matrix_Translate(cubos[j].x, cubos[j].y, cubos[j].z)
                            * Matrix_Scale(cubos[j].dx, cubos[j].dy, cubos[j].dz);

                    glm::vec4 bbox_max = model * glm::vec4(g_VirtualScene["cube"].bbox_max.x, g_VirtualScene["cube"].bbox_max.y, g_VirtualScene["cube"].bbox_max.z, 1.0f);
                    glm::vec4 bbox_min = model * glm::vec4(g_VirtualScene["cube"].bbox_min.x, g_VirtualScene["cube"].bbox_min.y, g_VirtualScene["cube"].bbox_min.z, 1.0f);

                    // If arrow collides with box
                    if(isPointInsideBBOX(arrows[i].pos, bbox_min, bbox_max))
                    {
                        engine->play2D("../../audio/plim.wav", false);
                        if(arrows[i].type == plataform)
                        {
                            plataformArrowPosition = nullptr;
                        }
                        else if(arrows[i].type == teleport && cubos[j].textureCode == CUBE2)
                        {
                            teleportPosition = arrows[i].pos;
                            TELEPORTED = true;
                            engine->play2D("../../audio/teleport.wav", false);
                        }
                        arrowCollides = true;
                    }

                }

                //Colisao com inimigos
                for(unsigned e = 0; e < enemies.size(); e++)
                {
                    model = Matrix_Translate(enemies[e].pos.x, enemies[e].pos.y, enemies[e].pos.z)
                            * Matrix_Scale(enemies[e].scale.x, enemies[e].scale.y + enemies[e].Y_deviation, enemies[e].scale.z);
                           // * Matrix_Rotate_Y(enemies[e].rotation_Y);
                    glm::vec4 bbox_max = model * glm::vec4(g_VirtualScene[enemies[e].name].bbox_max.x, g_VirtualScene[enemies[e].name].bbox_max.y, g_VirtualScene[enemies[e].name].bbox_max.z, 1.0f);
                    glm::vec4 bbox_min = model * glm::vec4(g_VirtualScene[enemies[e].name].bbox_min.x, g_VirtualScene[enemies[e].name].bbox_min.y, g_VirtualScene[enemies[e].name].bbox_min.z, 1.0f);

                    if(isPointInsideBBOX(arrows[i].pos, bbox_min, bbox_max))
                    {
                        engine->play2D("../../audio/dead.wav", false);
                        enemies.erase(enemies.begin() + e);
                        arrowCollides = true;
                    }

                }
                // Draw the arrow
                if(!arrowCollides)
                {
                    glm::vec4 uNew = crossproduct(camera_up_vector, - arrows[i].speed/norm(arrows[i].speed));
                    model = Matrix_Translate(arrows[i].pos.x, arrows[i].pos.y, arrows[i].pos.z)
                            * Matrix_Rotate(arrows[i].phiAngle, uNew)
                            * Matrix_Rotate(arrows[i].thetaAngle, camera_up_vector);
                    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                    if(arrows[i].type == teleport)
                        glUniform1i(object_id_uniform, ARROWT);
                    else if(arrows[i].type == plataform)
                        glUniform1i(object_id_uniform, ARROWP);
                    else
                        glUniform1i(object_id_uniform, ARROW);
                    DrawVirtualObject("arrow");
                }
                else
                {
                    arrows.erase(arrows.begin() + i);
                }

            }
            arrowRateController--;


            std::stringstream waveSST;
            waveSST << (int) (waveCounter);
            std::stringstream waveMaxSST;
            waveMaxSST << (int) (waveCounterLimit);
            std::string waveString = "wave: " + waveSST.str() + "/" + waveMaxSST.str();
            TextRendering_PrintString(window, waveString, -0.9f, 0.8f, 5.0f);


            ///DIALOGOS COMEÇAM AQUI

            if(reachedAnEnd)
            {
                if(glfwGetTime() - endBegin2 > 20)
                {
                    engine->stopAllSounds();
                    return;
                }
                printDialog(endDialog1, 25, endBegin1);
                printDialog(endDialog2, 25, endBegin2);
            }
            else
            {
                printDialog(beginDialog, 10, dialog1Start);
                printDialog(tutorialDialog, 30, tutorialBegin);
                endBegin1 = glfwGetTime();
                endBegin2 = endBegin1 + 30;
            }

        }

        // Doesn't allow more than 10 arrows in the scene
        if(arrows.size() > 10)
        {
            arrows.erase(arrows.begin(), arrows.begin() + 5);
        }

        glm::vec4 youBBoxMin = glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
        glm::vec4 youBBoxMax = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        youBBoxMax = Matrix_Translate(camera_position_c.x, camera_position_c.y, camera_position_c.z) * youBBoxMax;
        youBBoxMin = Matrix_Translate(camera_position_c.x, camera_position_c.y, camera_position_c.z) * youBBoxMin;

        // Collision Enemies with Yourself
        for(unsigned e = 0; e < enemies.size(); e++)
        {
            model = Matrix_Translate(enemies[e].pos.x, enemies[e].pos.y, enemies[e].pos.z)
                    * Matrix_Scale(enemies[e].scale.x, enemies[e].scale.y + enemies[e].Y_deviation, enemies[e].scale.z);

            glm::vec4 bbox_max = model * glm::vec4(g_VirtualScene[enemies[e].name].bbox_max.x, g_VirtualScene[enemies[e].name].bbox_max.y, g_VirtualScene[enemies[e].name].bbox_max.z, 1.0f);
            glm::vec4 bbox_min = model * glm::vec4(g_VirtualScene[enemies[e].name].bbox_min.x, g_VirtualScene[enemies[e].name].bbox_min.y, g_VirtualScene[enemies[e].name].bbox_min.z, 1.0f);

            if(areBBOXintersecting(youBBoxMin, youBBoxMax, bbox_min, bbox_max))
            {
                engine->play2D("../../audio/escalama.wav", false);
                camera_position_c.y = -1.0f; //kills you
            }

        }

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();

        whileTime = (glfwGetTime() * 10) - actualSecond;
    }
    engine->stopAllSounds();
    glfwSetWindowShouldClose(window, GL_FALSE);

}


// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slide 160 do documento "Aula_20_e_21_Mapeamento_de_Texturas.pdf"
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Parâmetros de amostragem da textura. Falaremos sobre eles em uma próxima aula.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene[object_name].first_index
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slide 217 e 219 do documento
// "Aula_03_Rendering_Pipeline_Grafico.pdf".
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage9"), 9);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage10"), 10);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage11"), 11);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage12"), 12);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage13"), 13);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage14"), 14);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gourad, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                if ( model->attrib.normals.size() >= (size_t)3*idx.normal_index )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( model->attrib.texcoords.size() >= (size_t)2*idx.texcoord_index )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = (void*)first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!

    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try
    {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    }
    catch ( std::exception& e )
    {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula (slides 32 até 40
    // do documento "Aula_07_Transformacoes_Geometricas_3D.pdf").
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slide 234
    // do documento "Aula_09_Projecoes.pdf".
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    //if (g_LeftMouseButtonPressed)
    //{

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Atualizamos parâmetros da câmera com os deslocamentos
    g_CameraTheta -= 0.005f*dx;
    g_CameraPhi   -= 0.005f*dy;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f/2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
    {
        g_CameraPhi = phimax;
    }
    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
    //}

    if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_ForearmAngleZ -= 0.01f*dx;
        g_ForearmAngleX += 0.01f*dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_TorsoPositionX += 0.01f*dx;
        g_TorsoPositionY -= 0.01f*dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if ( (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) && action == GLFW_PRESS)
    {
        enterPressed = true;
    }

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        g_ForearmAngleX = 0.0f;
        g_ForearmAngleZ = 0.0f;
        g_TorsoPositionX = 0.0f;
        g_TorsoPositionY = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        pressR = true;
        /*
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
        */
    }

    ///Teste de se apertou alguma das teclas WASD ou SPACE
    if ( (key == GLFW_KEY_W || key == GLFW_KEY_UP ) && action == GLFW_PRESS)
    {
        pressW = true;
    }

    if  (key == GLFW_KEY_A && action == GLFW_PRESS)
    {
        pressA = true;
    }

    if ( (key == GLFW_KEY_S || key == GLFW_KEY_DOWN ) && action == GLFW_PRESS)
    {
        pressS = true;
    }

    if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        pressD = true;
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        pressSpace = true;
        if(!JUMPING)
        {
            busyJUMPKey = true;
        }

    }

    ///Teste de se deixou de apertar alguma das teclas WASD ou SPACE
    if ( (key == GLFW_KEY_W || key == GLFW_KEY_UP ) && action == GLFW_RELEASE)
    {
        pressW = false;
        busyWKey = false;
    }

    if ((key == GLFW_KEY_S || key == GLFW_KEY_DOWN ) && action == GLFW_RELEASE)
    {
        pressS = false;
        busySKey = false;
    }

    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
    {
        pressA = false;
    }

    if (key == GLFW_KEY_D && action == GLFW_RELEASE)
    {
        pressD = false;
    }


    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        pressT = true;
    }
    if (key == GLFW_KEY_T && action == GLFW_RELEASE)
    {
        pressT = false;
    }

    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        pressE = true;
    }
    if (key == GLFW_KEY_E && action == GLFW_RELEASE)
    {
        pressE = false;
    }

    if (key == GLFW_KEY_R && action == GLFW_RELEASE)
    {
        pressR = false;
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
    {
        pressSpace = false;
        busyJUMPKey = false;
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     World", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     Camera", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                   NDC", -1.0f, 1.0f-13*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-14*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
    const tinyobj::attrib_t                & attrib    = model->attrib;
    const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
    const std::vector<tinyobj::material_t> & materials = model->materials;

    printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
    printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
    printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
    printf("# of shapes    : %d\n", (int)shapes.size());
    printf("# of materials : %d\n", (int)materials.size());

    for (size_t v = 0; v < attrib.vertices.size() / 3; v++)
    {
        printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.vertices[3 * v + 0]),
               static_cast<const double>(attrib.vertices[3 * v + 1]),
               static_cast<const double>(attrib.vertices[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.normals.size() / 3; v++)
    {
        printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.normals[3 * v + 0]),
               static_cast<const double>(attrib.normals[3 * v + 1]),
               static_cast<const double>(attrib.normals[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.texcoords.size() / 2; v++)
    {
        printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.texcoords[2 * v + 0]),
               static_cast<const double>(attrib.texcoords[2 * v + 1]));
    }

    // For each shape
    for (size_t i = 0; i < shapes.size(); i++)
    {
        printf("shape[%ld].name = %s\n", static_cast<long>(i),
               shapes[i].name.c_str());
        printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.indices.size()));

        size_t index_offset = 0;

        assert(shapes[i].mesh.num_face_vertices.size() ==
               shapes[i].mesh.material_ids.size());

        printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

        // For each face
        for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++)
        {
            size_t fnum = shapes[i].mesh.num_face_vertices[f];

            printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
                   static_cast<unsigned long>(fnum));

            // For each vertex in the face
            for (size_t v = 0; v < fnum; v++)
            {
                tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
                printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
                       static_cast<long>(v), idx.vertex_index, idx.normal_index,
                       idx.texcoord_index);
            }

            printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
                   shapes[i].mesh.material_ids[f]);

            index_offset += fnum;
        }

        printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.tags.size()));
        for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++)
        {
            printf("  tag[%ld] = %s ", static_cast<long>(t),
                   shapes[i].mesh.tags[t].name.c_str());
            printf(" ints: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j)
            {
                printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
                if (j < (shapes[i].mesh.tags[t].intValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");

            printf(" floats: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j)
            {
                printf("%f", static_cast<const double>(
                           shapes[i].mesh.tags[t].floatValues[j]));
                if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");

            printf(" strings: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j)
            {
                printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
                if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");
            printf("\n");
        }
    }

    for (size_t i = 0; i < materials.size(); i++)
    {
        printf("material[%ld].name = %s\n", static_cast<long>(i),
               materials[i].name.c_str());
        printf("  material.Ka = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].ambient[0]),
               static_cast<const double>(materials[i].ambient[1]),
               static_cast<const double>(materials[i].ambient[2]));
        printf("  material.Kd = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].diffuse[0]),
               static_cast<const double>(materials[i].diffuse[1]),
               static_cast<const double>(materials[i].diffuse[2]));
        printf("  material.Ks = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].specular[0]),
               static_cast<const double>(materials[i].specular[1]),
               static_cast<const double>(materials[i].specular[2]));
        printf("  material.Tr = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].transmittance[0]),
               static_cast<const double>(materials[i].transmittance[1]),
               static_cast<const double>(materials[i].transmittance[2]));
        printf("  material.Ke = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].emission[0]),
               static_cast<const double>(materials[i].emission[1]),
               static_cast<const double>(materials[i].emission[2]));
        printf("  material.Ns = %f\n",
               static_cast<const double>(materials[i].shininess));
        printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
        printf("  material.dissolve = %f\n",
               static_cast<const double>(materials[i].dissolve));
        printf("  material.illum = %d\n", materials[i].illum);
        printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
        printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
        printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
        printf("  material.map_Ns = %s\n",
               materials[i].specular_highlight_texname.c_str());
        printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
        printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
        printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
        printf("  <<PBR>>\n");
        printf("  material.Pr     = %f\n", materials[i].roughness);
        printf("  material.Pm     = %f\n", materials[i].metallic);
        printf("  material.Ps     = %f\n", materials[i].sheen);
        printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
        printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
        printf("  material.aniso  = %f\n", materials[i].anisotropy);
        printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
        printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
        printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
        printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
        printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
        printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
        std::map<std::string, std::string>::const_iterator it(
            materials[i].unknown_parameter.begin());
        std::map<std::string, std::string>::const_iterator itEnd(
            materials[i].unknown_parameter.end());

        for (; it != itEnd; it++)
        {
            printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
        }
        printf("\n");
    }
}

/// ---------------------------------------------------------------------------------------
///Funcoes implementadas pelo Andy

///Funcao que reseta a vida
void resetLife(float *novoX,float *novoZ,std::vector<Cubo> &cubos)
{

    CAINDO = false;
    *novoX = camera_position_c.x;
    *novoZ = camera_position_c.z;
    g_CameraTheta = INITIAL_THETA;
    g_CameraPhi = INITIAL_PHI;
    level = 1;
    cubos.clear();
    loadLevelPlatforms(cubos);

}

///Testa se uma coordenada de 1 dos eixos (x y ou z ) da camera esta entre o maior e menor valor dessa
///mesma coordenada do cubo passado
bool entreLimites(float posCamera, float eixo,float delta, float erro)
{
    bool entre = false;
    float sup = eixo + delta/2;
    float inf = eixo - delta/2;
    if(posCamera > inf && posCamera < sup + erro )
    {
        entre =  true;
    }
    return entre;
}

///Testa se estava acima da plataforma mas depois da gravidade fica "abaixo do chao"
bool caiuDemais(float antigoEixo,float novoEixo, float posEixo, float delta, float erro)
{
    if(antigoEixo >=  posEixo + delta/2 + erro &&
            entreLimites(novoEixo,posEixo,delta,erro) )
        return true;
    return false;
}

///Atualiza a posicao conforme as teclas de movimentos WASD
void processaWASD(float *novoX,float antigoX,float *novoZ,float antigoZ,float deslocamento,glm::vec4 u,glm::vec4 w)
{
    if (pressW)
    {
        (*novoX) = antigoX + whileTime*deslocamento*-w.x;
        (*novoZ) = antigoZ + whileTime*deslocamento*-w.z;
    }
    if (pressS)
    {
        (*novoX) = antigoX + whileTime*deslocamento*w.x;
        (*novoZ) = antigoZ + whileTime*deslocamento*w.z;
    }

    if (pressA)
    {
        (*novoX) = antigoX + whileTime*deslocamento*-u.x;
        (*novoZ) = antigoZ + whileTime*deslocamento*-u.z;
    }
    if (pressD)
    {
        (*novoX) = antigoX + whileTime*deslocamento*u.x;
        (*novoZ) = antigoZ + whileTime*deslocamento*u.z;
    }

}

///Atualiza quando comeca/acaba o pulo e doublepulo
void atualizaPulo()
{
    if (!JUMPING &&!CAINDO)
    {
        JUMPING = true;
        startJump = actualSecond;

    }
    else if(JUMPING && !DOUBLEJUMPING && !busyJUMPKey)
    {
        DOUBLEJUMPING = true;
        startJump = actualSecond;
    }
}

///Se o personagem esta caminhando na plataforma sem pular, testa quando ele cai ou ainda permanece sobre a plataforma
void testaChao(float novoX,float novoZ,unsigned int *startFall,unsigned int actualSecond,std::vector<Cubo> cubos)
{
    CAINDO = true;
    for(int i = 0; i < (int) cubos.size() ; i++)
    {
        if((entreLimites(novoX,cubos[i].x,cubos[i].dx,0.0f))
                &&(entreLimites(novoZ,cubos[i].z,cubos[i].dz,0.0f))
                &&(entreLimites(camera_position_c.y,cubos[i].y,cubos[i].dy,ALTURAHERO)))

        {
            CAINDO = false;
            *startFall = actualSecond;
            break;

        }
    }
}



///Funcao principal de tratar colisao. Deriva em varias funcoes menores para cada ocasiao especifica
void trataColisaoCubo(float novoX,float antigoY,float novoZ, int nearestCube,bool *invadiuObjeto,bool *pousou,std::vector<Cubo> cubos)
{
    ///Se ele permanecceu nos limites do mesmo cubo, ou ele está andando sobre ele ou apenas está caindo ainda na zona delimitada pelo cubo
    if(nearestCube == oldCubo)
    {
        ///Se tem variacao no Y pode ter pousado pois estaav caindo
        if(antigoY != camera_position_c.y && antigoY >= cubos[nearestCube].y + (cubos[nearestCube].dy)/2 + ALTURAHERO - 0.1f )
        {
            (*pousou) = processaPouso(antigoY,cubos[nearestCube].y,cubos[nearestCube].dy,ALTURAHERO);
        }
        else
        {
            ///Pode apenas estar andando sobre a plataforma, ai só testa se ele nao bate em outra plataforma
            if (entreLimites(camera_position_c.y,cubos[nearestCube].y,cubos[nearestCube].dy,ALTURAHERO -0.01f))
            {
                *invadiuObjeto = true;
            }
        }
    }
    else
    {
        ///Se antes ele estava sobre o "nada" (estava pulando e aos seus pes nao havia nada)
        if(oldCubo == -1)
        {
            ///Testa se pousou sobre uma plataforma
            if (processaPouso(antigoY,cubos[nearestCube].y,cubos[nearestCube].dy,ALTURAHERO -0.01f))
            {

            }
            else
            {
                ///Testa se bateu de frente numa plataforma pois nao caiu acima dela
                if (entreLimites(camera_position_c.y,cubos[nearestCube].y,cubos[nearestCube].dy,ALTURAHERO -0.01f))
                {

                    *invadiuObjeto = true;
                }

            }
        }
        else
        {
            ///O novo movimento fez ele trocar o cubo mais proximo dele, testa se nao invadiu o cubo entrando nele
            if (entreLimites(camera_position_c.y,cubos[nearestCube].y,cubos[nearestCube].dy,ALTURAHERO -0.01f))
            {
                *invadiuObjeto = true;
            }
        }


    }
}

///Retorna o indice da plataforma mais proxima da camera que esteja sendo invadido sua BBox no vetor de plataformas passado
int getNearestCube(float novoX,float antigoY,float novoZ,std::vector<Cubo> cubos)
{
    int indice = -1;
    float longe = 100;
    float aux;
    for(int i = 0; i < (int)cubos.size() ; i++)
    {
        if(entreLimites(novoX,cubos[i].x,cubos[i].dx,0.0f) )
        {
            if(entreLimites(novoZ,cubos[i].z,cubos[i].dz,0.0f) )
            {
                aux = camera_position_c.y - (cubos[i].y + cubos[i].dy/2 + ALTURAHERO );
                if(  aux < longe )
                {
                    indice = i;
                    longe = aux;

                }

            }
        }
    }
    return indice;
}

/// Caso o personagem parar de estar "voando sobre o vazio" testa algumas possiveis colisoes
void processaColisao(float novoX,float antigoY,float novoZ,bool *invadiuObjeto,std::vector<Cubo> cubos)
{
    int nearestCube = getNearestCube(novoX,antigoY,novoZ,cubos);
    bool pousou = false;
    if (nearestCube!= -1)
    {
        trataColisaoCubo(novoX,antigoY,novoZ,nearestCube,invadiuObjeto,&pousou,cubos);
        if (pousou){
            testCheckPoint(nearestCube,cubos);
        }

    }
    if (*invadiuObjeto == true)
    {
        cubos.erase(cubos.begin() + nearestCube);
        float nearestCubeBelow = getNearestCube(novoX,antigoY,novoZ,cubos);
        trataColisaoCubo(novoX,antigoY,novoZ,nearestCubeBelow,invadiuObjeto,&pousou,cubos);
        nearestCube = nearestCubeBelow;
    }

    oldCubo = nearestCube;

}

///Processa o movimento de ir caindo ate encostar na plataforma
bool processaPouso(float antigoY,float cuboY,float cuboDY,float correcao)
{
    if(caiuDemais(antigoY,camera_position_c.y,cuboY,cuboDY,correcao)) ///Testa se caiu mais do que deveria na plataforma
    {
        camera_position_c.y =(cuboY + cuboDY/2) + correcao ;
        CAINDO = false;
        return true;
    }
    return false;
}

///Carrega as plataformas segunda o nivel que for
void loadLevelPlatforms(std::vector<Cubo> &cubos){
    if (level == 1){
        //camera_position_c = glm::vec4(cubos[16].x, cubos[16].y + cubos[16].dy/2 + ALTURAHERO, cubos[16].z, 1.0f);
        camera_position_c = glm::vec4(cubos[1].x, cubos[1].y + cubos[1].dy/2 + ALTURAHERO, cubos[1].z, 1.0f);
        loadFirstMap(cubos);
    }
    if (level == 2){
        loadSecondMap(cubos);
        camera_position_c = glm::vec4(cubos[1].x , cubos[1].y + 30, cubos[1].z - 15, 1.0f);
        //camera_position_c = glm::vec4(cubos[18].x, cubos[18].y + cubos[18].dy/2 + ALTURAHERO, cubos[18].z, 1.0f);
    }

    if(level == 3){
        loadThirdMap(cubos);
        camera_position_c = glm::vec4(cubos[1].x , cubos[1].y + 30, cubos[1].z - 15, 1.0f);
        //camera_position_c = glm::vec4(cubos[18].x, cubos[18].y + 30, cubos[18].z, 1.0f);
    }

    if(level == 4){
        loadFourthMap(cubos);
        camera_position_c = glm::vec4(cubos[1].x, cubos[1].y + 30, cubos[1].z, 1.0f);
    }
}

/// Sepa processa os Movimentos, desconfio pelo nome da funcao
void processaMovimentos(bool WASD,float antigoX,float * novoX,float antigoZ,float * novoZ,float antigoY,std::vector<Cubo> &cubos, glm::vec4 teleportPos)
{
    antigoY = camera_position_c.y;

    if(TELEPORTED)
        handleTeleport(novoX, novoZ, cubos, teleportPos);

    if(WASD || CAINDO || JUMPING || MAGICPLATFORM || DIED)
    {
        bool invadiuObjeto = false;
        MAGICPLATFORM = false;

        ///Se nao ta caindo nem pulando so caminhou. Testa se ele nao saiu da plataforma e deveria cair
        if(!CAINDO && !JUMPING)
        {
            testaChao(*novoX,*novoZ,&startFall,actualSecond,cubos);
        }
        ///Se pulou e/ou caiu ja aplica gravidade
        else
        {
            aplicaGravidade();
        }

        ///Processa colisao com objetos
        processaColisao(*novoX,antigoY,*novoZ,&invadiuObjeto,cubos);

        ///So reseta a vida quando esta morto E aperta enter
        if(DIED && enterPressed)
        {
            enterPressed = false;
            resetLife(novoX,novoZ,cubos);
            CAINDO = false;
        }
        ///So muda a posicao do X e do Z se o movimento for permitido, ou seja, nao foi proibido por invadir um objeto
        else if(!invadiuObjeto)
        {
            camera_position_c.x = *novoX;
            camera_position_c.z = *novoZ;
        }

    }
}

///Seleciona a flecha a retornar dependendo botao apertada
ArrowType selectArrowType()
{
    if(pressE)
    {
        return normal;
    }
    else if(pressR)
    {
        return plataform;
    }
    else if(pressT)
    {
        return teleport;
    }
    else
    {
        return normal;
    }
}

///Trata o teleport com a flecha
void handleTeleport(float * novoX,float * novoZ, std::vector<Cubo> cubos, glm::vec4 teleportPos)
{
    int teleportedPlatform = getNearestCube(teleportPos.x,teleportPos.y,teleportPos.z,cubos);
    //cout <<"cubo onde estou em cima : "<< teleportedPlatform << endl;
    if (teleportedPlatform != -1)
    {
        camera_position_c.x = cubos[teleportedPlatform].x;
        camera_position_c.z = cubos[teleportedPlatform].z;
        camera_position_c.y =(cubos[teleportedPlatform].y + cubos[teleportedPlatform].dy/2) + ALTURAHERO;
        *novoX = camera_position_c.x;
        *novoZ = camera_position_c.z;

        CAINDO = false;
        JUMPING = false;
        DOUBLEJUMPING = false;
        TELEPORTED = false;
    }

}

///Essa aqui ta dificil... Pelo nome da funcao nao consigo deduzir
void aplicaGravidade()
{
    if (JUMPING)
    {
        camera_position_c.y +=  1.2*whileTime;
        if(actualSecond - startJump > 3)
        {
            JUMPING = false;
            DOUBLEJUMPING = false;
            CAINDO = true;
            startFall = actualSecond;
        }
    }

    else
    {
        camera_position_c.y -=0.5*whileTime;
    }

}

///Testa se chegou num ponto onde mudam as coisas
void testCheckPoint(int nearestCube,std::vector<Cubo> cubos){
    if(level == 1 && nearestCube == 17){
        endingPlatform = true;
        level+=1;
    }

    if(level == 2 && nearestCube == 21 ){
        cout << "entrei 2 " << endl;
        endingPlatform = true;
        level+=1;
    }

    if(level == 3 && nearestCube == 19){
        cout << "yay"<< endl;
        endingPlatform = true;
        level+=1;
    }
}

///Carrega posicoes dos cubos.
void loadFirstMap(std::vector<Cubo> &cubos)
{
    cubos.push_back(Cubo(0,0,0,10.0f,1.0f,10.0f,ARROWP));
    cubos.push_back(Cubo(0.0f,0.0f,0.0f,10.0f,1.0f,10.0f,ARROWP));
    cubos.push_back(Cubo(-40.5f, 0.0f, -1.5f, 60.9f, 2.0f, 15.0f,CUBE1));
    cubos.push_back(Cubo(-105.5f, 1.0f, -1.5f, 60.9f, 2.0f, 15.0f,CUBE1));
    cubos.push_back(Cubo(-170.5f, 1.8f, -1.5f, 60.9f, 2.0f, 15.0f,CUBE1));
    cubos.push_back(Cubo(-235.5f, 2.6f, -1.5f, 60.9f, 2.0f, 15.0f,CUBE1));

    cubos.push_back(Cubo(-280.5f, 3.1f, -1.5f, 19.9f, 2.0f, 5.0f,CUBE));
    cubos.push_back(Cubo(-310.5f, 3.1f, 5.5f, 19.9f, 2.0f, 5.0f,CUBE));
    cubos.push_back(Cubo(-334.5f, 3.1f, -4.5f, 19.9f, 2.0f, 5.0f,CUBE));

    cubos.push_back(Cubo(-360.5f, 3.1f, 4.5f, 19.9f, 2.0f, 5.0f,CUBE));
    cubos.push_back(Cubo(-395.5f, 2.1f, -2.5f, 19.9f, 2.0f, 5.0f,CUBE));
    cubos.push_back(Cubo(-425.5f, 2.8f, 4.5f, 19.9f, 2.0f, 5.0f,CUBE));

    cubos.push_back(Cubo(-458.5f, 3.5f, -1.5f, 19.9f, 2.0f, 5.0f,CUBE));
    cubos.push_back(Cubo(-493.5f, 3.5f, 3.5f, 19.9f, 2.0f, 5.0f,CUBE));

    cubos.push_back(Cubo(-509.5f, 3.5f, 21.5f, 5.0f, 2.0f, 19.9f,CUBE));
    cubos.push_back(Cubo(-500.5f, 3.5f, 50.5f, 5.0f, 2.0f, 19.9f,CUBE));
    cubos.push_back(Cubo(-518.5f, 3.5f, 69.5f, 5.0f, 2.0f, 19.9f,CUBE));

    cubos.push_back(Cubo(-518.5f, 0.5f, 93.5f, 16.6f, 2.0f, 10.0f,CUBE3));

}

///Carrega posicoes dos cubos.
void loadSecondMap(std::vector<Cubo> &cubos)
{


    cubos.push_back(Cubo(0.0f,0.0f,0.0f,10.0f,1.0f,10.0f,ARROWP));
    cubos.push_back(Cubo(-300.5f, 50.0f, -1.5f, 25.0f, 2.0f, 25.0f,CUBE1));

    cubos.push_back(Cubo(-300.5f, 50.0f, 25.5f, 17.5f, 2.0f, 5.0f,CUBE1));
    cubos.push_back(Cubo(-300.5f, 51.4f, 40.5f, 17.5f, 2.0f, 5.0f,CUBE1));
    cubos.push_back(Cubo(-300.5f, 52.8f, 55.5f, 17.5f, 2.0f, 5.0f,CUBE1));
    cubos.push_back(Cubo(-300.5f, 54.2f, 70.5f, 17.5f, 2.0f, 5.0f,CUBE1));
    cubos.push_back(Cubo(-300.5f, 55.6f, 85.5f, 17.5f, 2.0f, 5.0f,CUBE1));
    cubos.push_back(Cubo(-300.5f, 57.0f, 110.5f, 17.5f, 2.0f, 30.0f,CUBE1));


    cubos.push_back(Cubo(-306.5f, 58.5f, 130.5f, 2.5f, 5.0f, 2.0f,CUBE));
    cubos.push_back(Cubo(-308.5f, 60.0f, 140.5f, 2.5f, 5.0f, 2.0f,CUBE));
    cubos.push_back(Cubo(-308.5f, 62.0f, 149.5f, 2.5f, 5.0f, 2.0f,CUBE));
    cubos.push_back(Cubo(-300.5f, 64.0f, 146.5f, 2.5f, 5.0f, 2.0f,CUBE));
    cubos.push_back(Cubo(-294.5f, 66.0f, 138.5f, 2.5f, 5.0f, 2.0f,CUBE));
    cubos.push_back(Cubo(-290.5f, 68.0f, 130.5f, 2.5f, 5.0f, 2.0f,CUBE));
    cubos.push_back(Cubo(-300.5f, 70.0f, 130.5f, 2.5f, 5.0f, 2.0f,CUBE));
    cubos.push_back(Cubo(-300.5f, 72.0f, 140.5f, 2.5f, 5.0f, 2.0f,CUBE));
    cubos.push_back(Cubo(-300.5f, 74.0f, 152.5f, 2.5f, 5.0f, 2.0f,CUBE));

    cubos.push_back(Cubo(-300.5f, 75.4f, 170.5f, 17.5f, 2.0f, 25.0f,CUBE1));
    cubos.push_back(Cubo(-300.5f, 75.4f, 220.5f, 17.5f, 2.0f, 25.0f,CUBE1));
    cubos.push_back(Cubo(-330.5f, 75.4f, 195.5f, 17.5f, 2.0f, 25.0f,CUBE1));
    cubos.push_back(Cubo(-270.5f, 75.4f, 195.5f, 17.5f, 2.0f, 25.0f,CUBE1));

    cubos.push_back(Cubo(-300.5f, 60.5f, 195.5f, 16.6f, 2.0f, 10.0f,CUBE3));


}
///Carrega posicoes dos cubos.
void loadThirdMap(std::vector<Cubo> &cubos){
    cubos.push_back(Cubo(0.0f,0.0f,0.0f,10.0f,1.0f,10.0f,ARROWP));
    cubos.push_back(Cubo(-100.5f, 45.0f, -1.5f, 25.0f, 2.0f, 25.0f,CUBE1));

    cubos.push_back(Cubo(-160.5f, 45.0f, -1.5f, 15.0f, 2.0f, 15.0f,CUBE2));
    cubos.push_back(Cubo(-200.5f, 45.0f, 15.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-240.5f, 45.0f, -25.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-270.5f, 45.0f, 30.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-270.5f, 50.0f, 60.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-230.5f, 52.0f, 80.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-190.5f, 56.0f, 125.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-180.5f, 60.0f, 195.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-150.5f, 60.0f, 240.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-110.5f, 60.0f, 260.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-150.5f, 60.0f, 325.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-180.5f, 60.0f, 365.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-250.5f, 60.0f, 420.5f, 8.0f, 2.0f, 8.0f,CUBE2));

    cubos.push_back(Cubo(-380.5f, 60.0f, 420.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-410.5f, 60.0f, 420.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-395.5f, 60.0f, 390.5f, 8.0f, 2.0f, 8.0f,CUBE2));
    cubos.push_back(Cubo(-395.5f, 60.0f, 450.5f, 8.0f, 2.0f, 8.0f,CUBE2));

    cubos.push_back(Cubo(-395.5f, 45.0f, 420.5f, 8.0f, 2.0f, 8.0f,CUBE3));
}
///Carrega posicoes dos cubos.
void loadFourthMap(std::vector<Cubo> &cubos){
    cubos.push_back(Cubo(0.0f,0.0f,0.0f,10.0f,1.0f,10.0f,ARROWP));
    cubos.push_back(Cubo(-255.5f, 50.0f, 200.5f, 40.9f, 2.0f, 45.0f,CUBE1));
}

///Retorna a quantidade de inimigos depdendendo a fase que for
int getLevelEnemies(int waveCounter){
    int num = 0;
    switch(waveCounter){
    case -1:
        num = 4;
        break;
    case 0:
        num = 8;
        break;
    case 1:
        num = 10;
        break;
    case 2:
        num = 12;
        break;
    }

    return num;

}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

///Draw Background
void drawBackground(glm::vec4 &wallpaperPos){

        model = Matrix_Translate(wallpaperPos.x, wallpaperPos.y + 250, wallpaperPos.z)
                *Matrix_Rotate_Z(3.141592)
                *Matrix_Scale(500,1,500);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALLPAPER);
        DrawVirtualObject("cube");
        model = Matrix_Translate(wallpaperPos.x + 250, wallpaperPos.y, wallpaperPos.z)
                *Matrix_Rotate_Z(3.141592)
                *Matrix_Scale(1,500,500);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALLPAPER);
        DrawVirtualObject("cube");
        model = Matrix_Translate(wallpaperPos.x - 250, wallpaperPos.y, wallpaperPos.z)
                *Matrix_Rotate_Z(3.141592)
                *Matrix_Scale(1,500,500);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALLPAPER);
        DrawVirtualObject("cube");
        model = Matrix_Translate(wallpaperPos.x, wallpaperPos.y, wallpaperPos.z + 250)
                *Matrix_Rotate_Z(3.141592)
                *Matrix_Scale(500,500,1);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALLPAPER);
        DrawVirtualObject("cube");
        model = Matrix_Translate(wallpaperPos.x, wallpaperPos.y, wallpaperPos.z - 250)
                *Matrix_Rotate_Z(3.141592)
                *Matrix_Scale(500,500,1);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALLPAPER);
        DrawVirtualObject("cube");
}
