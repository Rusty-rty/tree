#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>
#include <math.h>

// globals

float min_branch_angle = -45;
float max_branch_angle = 45;
float max_start_branch_len = 5;
float min_start_branch_len = 1;
float length_grow_speed = 2;
float length_grow_factor = 2;
float new_branch_factor = 2;

float depth_correction_factor = 1.0 / 3.0;
float brightness_correction_div = 0.5;

int color_mutation = 1;

float dead_angle_zone = 3;

int generation_time = 75;

float leaf_size_growth_correction = 7;

unsigned int max_branches_num = 5;
unsigned int max_leaf_draw_branch_num = 2;

float wait_time = 100;

float branch_symbol_correction_div = 0.7;

unsigned int seed = 0;

bool run_simulation = true;

unsigned int screen_width = 150;
unsigned int screen_height = 40;

char *branch_symbols = " .'`^\",:;Il!i~+?][}{1)(|\\/*tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%%B@$";

float leaf_symbol_correction_div = 4;

char *leaf_symbols = "_==%%#@";

bool wait_enabled = true;
bool print_enabled = true;
bool clear_enabled = true;

int end_generation_wait = 1000;

double PI = 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067;

// utils

void nullptrexit(void *ptr)
{
    if (ptr == NULL)
    {
        printf("Nullprtexeption\n");
        exit(1);
    }
}

int nulltoone(int num)
{
    if (num == 0)
        num = 1;
    return num;
}

void clear_screen()
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

int rand_color()
{
    return rand() % 8;
}

struct Leaf;

typedef struct Leaf
{
    int age;
    float r;
    int base_color;
} Leaf;

struct Ast;

typedef struct Ast
{
    int base_color;
    unsigned int age;
    float angle;
    unsigned int len;

    unsigned int children_num;

    struct Ast **children;

    struct Leaf *leaf;

} Ast;

typedef struct Tree
{
    int leaf_color;

    unsigned int age;

    unsigned int x;
    unsigned int y;

    unsigned int asts_num;

    Ast *asts;

    Leaf pot;

} Tree;

typedef struct Pixel
{
    char c;
    WORD color;

} Pixel;

typedef struct Screen
{
    unsigned int w;
    unsigned int h;

    Pixel *screen;

    CHAR_INFO *prt_buffer;

} Screen;

void leaf_init(Leaf *leaf, int color)
{
    leaf->base_color = color;
    leaf->age = 1;
    leaf->r = 1;
}

void leaf_step(Leaf *leaf)
{
    leaf->age += 1;

    if (rand() % (int)((float)leaf->age * leaf_size_growth_correction) == 0)
        leaf->r += 1;
}

void leaf_draw(Leaf *leaf, Screen *screen, int in_x, int in_y)
{

    int r = (int)leaf->r;

    float aspect = 0.5f;

    for (int dy = -r; dy <= r; dy++)
    {
        for (int dx = -r; dx <= r; dx++)
        {

            float dist =
                dx * dx +
                (dy * aspect) * (dy * aspect);

            if (dist <= r * r)
            {

                int x = in_x + dx;
                int y = in_y + dy;

                if (x >= 0 && x < screen->w &&
                    y >= 0 && y < screen->h)
                {

                    int look_index = leaf->age / leaf_symbol_correction_div;

                    if (look_index >= strlen(leaf_symbols))
                        look_index = strlen(leaf_symbols) - 1;

                    char symbol = leaf_symbols[look_index];

                    screen->screen[y * screen->w + x].c = symbol;

                    float age_factor = leaf->age;

                    int brightness = (age_factor) / brightness_correction_div;

                    if (brightness < 0)
                        brightness = 0;

                    if (brightness > 8)
                        brightness = 8;

                    WORD final_color = leaf->base_color | brightness;

                    screen->screen[y * screen->w + x].color = final_color;
                }
            }
        }
    }
}

void ast_init(Ast *ast, int parent_color, int leaf_color)
{

    ast->len = rand() % (int)(max_start_branch_len - min_start_branch_len + 1) + min_start_branch_len;

    ast->age = 1;

    ast->angle = rand() % (int)(max_branch_angle - min_branch_angle + 1) + min_branch_angle;

    while (1)
    {
        if (ast->angle < max_branch_angle / dead_angle_zone &&
            ast->angle > min_branch_angle / dead_angle_zone)
        {
            ast->angle = rand() % (int)(max_branch_angle - min_branch_angle + 1) + min_branch_angle;
            continue;
        }

        break;
    }

    ast->children_num = 0;

    int base = parent_color & 7;

    base += ((rand() % color_mutation) - 1);

    base %= 8;

    ast->base_color = rand_color();

    ast->children = malloc(max_branches_num * sizeof(Ast *));

    nullptrexit(ast->children);

    ast->leaf = malloc(sizeof(Leaf));

    leaf_init(ast->leaf, leaf_color);
}

void ast_free(Ast *ast)
{
    for (int i = 0; i < ast->children_num; i++)
    {
        ast_free(ast->children[i]);

        free(ast->children[i]);
    }

    free(ast->children);

    free(ast->leaf);
}

void ast_step(Ast *ast, int depth, int leaf_color)
{

    if (ast->children_num > max_branches_num)
    {
        exit(2);
    }

    ast->age += 1;

    float factor = depth * depth_correction_factor;

    if (rand() % (int)(ast->age * length_grow_factor * factor) == 0)
        ast->len += 1;

    if (rand() % nulltoone(ast->age * new_branch_factor * nulltoone(depth * depth_correction_factor)) == 0)
    {
        if (ast->children_num < max_branches_num)
        {
            Ast *ast_ptr = malloc(sizeof(Ast));

            nullptrexit(ast_ptr);

            ast_init(ast_ptr, ast->base_color, leaf_color);

            ast->children[ast->children_num] = ast_ptr;

            ast->children_num++;
        }
    }

    leaf_step(ast->leaf);

    for (int i = 0; i < ast->children_num; i++)
    {
        ast_step(ast->children[i], depth + 1, leaf_color);
    }
}

void ast_draw(Ast *ast, Screen *screen, int x, int y, int angle, int depth)
{

    double rad = (ast->angle + angle) * PI / 180.0;

    double dx = sin(rad);
    double dy = -cos(rad);

    double px = x;
    double py = y;

    for (int i = 0; i < ast->len; i++)
    {

        int draw_x = (int)(px + 0.5);
        int draw_y = (int)(py + 0.5);

        if (draw_x >= 0 && draw_x < screen->w &&
            draw_y >= 0 && draw_y < screen->h)
        {

            int look_index = ast->age / branch_symbol_correction_div;

            if (look_index >= strlen(branch_symbols))
                look_index = strlen(branch_symbols) - 1;

            char symbol = branch_symbols[look_index];

            screen->screen[draw_y * screen->w + draw_x].c = symbol;

            float age_factor = ast->age;
            float depth_factor = depth * 0.3f;

            int brightness = (age_factor - depth_factor) / brightness_correction_div;

            if (brightness < 0)
                brightness = 0;

            if (brightness > 8)
                brightness = 8;

            WORD final_color = ast->base_color | brightness;

            screen->screen[draw_y * screen->w + draw_x].color = final_color;
        }

        px += dx;
        py += dy;
    }

    int end_x = (int)(px + 0.5);
    int end_y = (int)(py + 0.5);

    if (ast->children_num < max_leaf_draw_branch_num)
        leaf_draw(ast->leaf, screen, end_x, end_y);

    for (int i = 0; i < ast->children_num; i++)
    {
        ast_draw(ast->children[i], screen, end_x, end_y, ast->angle + angle, depth + 1);
    }
}

void tree_init(Tree *tree)
{

    tree->leaf_color = rand_color();

    tree->asts = malloc(max_branches_num * sizeof(Ast));

    tree->age = 1;

    tree->x = screen_width / 2;
    tree->y = screen_height - 1;

    tree->asts_num = 1;

    ast_init(&tree->asts[0], rand_color(), tree->leaf_color);

    leaf_init(&tree->pot, rand_color());
}

void tree_grow(Tree *tree)
{

    tree->age += 1;

    if (rand() % nulltoone(tree->age) == 0)
    {
        if (tree->asts_num < max_branches_num)
        {
            Ast new_ast;

            ast_init(&new_ast, rand_color(), tree->leaf_color);

            tree->asts[tree->asts_num] = new_ast;

            tree->asts_num++;
        }
    }

    for (int i = 0; i < tree->asts_num; i++)
    {
        ast_step(&tree->asts[i], 1, tree->leaf_color);
    }

    leaf_step(&tree->pot);
}

void tree_draw(Tree *tree, struct Screen *screen)
{

    for (int i = 0; i < screen->w * screen->h; i++)
    {
        screen->screen[i].c = ' ';
        screen->screen[i].color = 1;
    }

    for (int i = 0; i < tree->asts_num; i++)
    {
        ast_draw(&tree->asts[i], screen, tree->x, tree->y, 0, 1);
    }

    int index = tree->x + tree->y * screen->w;

    if (index < screen->w * screen->h && index > 0)
    {
        screen->screen[index].c = 'S';
        screen->screen[index].color = rand_color();
    }

    leaf_draw(&tree->pot, screen, tree->x, tree->y);
}

void tree_free(Tree *tree)
{
    for (int i = 0; i < tree->asts_num; i++)
    {
        ast_free(&tree->asts[i]);
    }

    free(tree->asts);
}

void screen_init(Screen *screen)
{
    screen->w = screen_width;
    screen->h = screen_height;

    screen->screen = malloc(screen_width * screen_height * sizeof(Pixel));

    nullptrexit(screen->screen);

    screen->prt_buffer = malloc(sizeof(CHAR_INFO) * screen->w * screen->h);

    nullptrexit(screen->prt_buffer);
}

void screen_free(Screen *screen)
{
    free(screen->screen);
    free(screen->prt_buffer);
}

void screen_prt(Screen *screen)
{

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    COORD bufferSize = {screen->w, screen->h};

    COORD bufferCoord = {0, 0};

    SMALL_RECT writeRegion = {0, 0, screen->w - 1, screen->h - 1};

    for (int i = 0; i < screen->w * screen->h; i++)
    {
        screen->prt_buffer[i].Char.AsciiChar = screen->screen[i].c;
        screen->prt_buffer[i].Attributes = screen->screen[i].color;
    }

    WriteConsoleOutputA(
        hConsole,
        screen->prt_buffer,
        bufferSize,
        bufferCoord,
        &writeRegion);
}

void draw_info(Screen *screen, const Tree *tree)
{

    char str[12];

    sprintf(str, "%d", seed);

    for (int i = 0; i < strlen(str); i++)
    {
        int index = i + 5;

        if (index < screen->w * screen->h && index > 0)
            screen->screen[index].c = str[i];
    }

    char str2[12];

    sprintf(str2, "%d", tree->age);

    for (int i = 0; i < strlen(str2); i++)
    {
        int index = i + screen->w + 6;

        if (index < screen->w * screen->h && index > 0)
            screen->screen[index].c = str2[i];
    }
}

void sim(Screen *screen)
{

    srand(seed);

    Tree tree;

    tree_init(&tree);

    for (int i = 0; i < generation_time; i++)
    {

        clear_screen();

        tree_grow(&tree);

        tree_draw(&tree, screen);

        draw_info(screen, &tree);

        if (print_enabled)
            screen_prt(screen);

        if (wait_enabled)
            Sleep(wait_time);
    }

    Sleep(end_generation_wait);

    tree_free(&tree);

    if (clear_enabled)
        system("cls");
}

int main()
{

    Screen screen;

    screen_init(&screen);

    while (run_simulation)
    {

        srand(time(NULL));

        seed = rand();

        sim(&screen);
    }

    screen_free(&screen);

    return 0;
}