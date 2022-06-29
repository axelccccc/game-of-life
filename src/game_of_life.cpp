/**
 * @file game_of_life.cpp
 * @author axelccccc (github.com/axelccccc)
 * @brief CLI implementation of John Conway's Game Of Life
 * @version 0.2
 * @date 2022-03-20
 * 
 * @copyright Copyright (c) axelccccc — 2022
 * 
 */

/**
 * CHANGELOG
 * 
 * 0.2 : 
 *  - parses arguments with `getopt`
 *  - adds time monitoring capability
 *  - optimizes calculation times using multithreading
 * 
 */

 /**
 * BUGS:
 *  - Alignment does not work
 * TODO:
 * 
 */

#include <exception>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <utility>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <getopt.h>






// #define DEBUG
// #define MEASURE_TIME

#define REFRESH_RATE    40ms    // 40ms Best speed / stability compromise (& lowers check() time compared to 30ms)
#define NUM_THREADS     4       // 4 optimal — reduces check() time from ~1ms to ~0.55ms


using namespace std::chrono_literals;

/**
 * @brief typedef for a grid row 
 * NOTE: using std::string instead cancels the speed gains of
 * using multiple threads ? but way more stable printing
 */
using row_t = std::vector<char>;

/**
 * @brief typedef for a raw grid
 */
using grid_t = std::vector<row_t>;

/**
 * @brief Alignment requested when loading a grid
 * into a bigger one
 */
enum GridAlignment {
    none = 0,
    center,
    top_left,
    top_right,
    bottom_left,
    bottom_right
};

/**
 * @brief CLI usage message
 * @param exe executable path (`argv[0]`)
 */
void usage(const char* exe) {
    printf("\
Usage : %s [-p <particle>] [-a <alignment>] <file.txt>\n\
\n\
\t-p\tcharacter to be used as cell graphics\n\
\t-a\talignment of seed file in display (center, top-left, bottom-right, ...)\n\
\n\
\t<file.txt>\tseed file\n\
", exe);
}






/* PROTOTYPES */

class Chronometer {
public:

    // Chronometer() = default;
    // ~Chronometer() = default;

    void start() {
        t_start = std::chrono::high_resolution_clock::now();
    };

    void stop() {
        t_stop = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration_ms = t_stop - t_start;
        durations_ms.push_back(duration_ms.count());
    };

    [[nodiscard]] const double& avg() const { return duration_avg; }

    void update_avg() {
        duration_avg = std::accumulate(durations_ms.begin(), durations_ms.end(), 0.0) / durations_ms.size();
    }

private:

    std::chrono::time_point<std::chrono::steady_clock> t_start, t_stop;
    std::vector<double> durations_ms;
    double duration_avg = 0.0;
    
};


#ifdef MEASURE_TIME
static Chronometer ms_chrono;
#endif

/**
 * @brief Two dimensional grid of characters
 * 
 */
class Grid {
public:

    Grid(int height, int length);

    Grid(grid_t grid)
    : grid(std::move(grid)) {}

    Grid(int height, int length, Grid&& other, GridAlignment align);


    row_t& operator[](int key) { return this->grid[key]; }

    int height() const { return grid.size(); }
    int width()  const { return grid[0].size(); }

    bool operator==(const Grid& other) const { return this->grid == other.grid; }

    void print();

private:

    grid_t grid;

};






/**
 * @brief Parameter structure for a Game Of Life.
 * Used mainly to parse CLI arguments
 */
struct GameOfLifeParams {

    int height = 0;
    int width = 0;
    char particle = '*';
    const char* filepath = nullptr;
    GridAlignment alignment = center;
    
};






/**
 * @brief Object representing a game of life
 * and its data
 */
class GameOfLife {
public:

    GameOfLife(int x, int y, char particle = '*');
    GameOfLife(Grid start_grid, char particle = '*');
    GameOfLife(GameOfLifeParams params);

    void start();

private:

    Grid grid;
    Grid next_grid;

    char particle;

    bool stale = false;

    void display();
    void update();
    void check();
    void check_bounded(int height, int width, int start_x, int start_y);
    void check_multithread(int num_threads);
    int check_neighbors(int x, int y);

    void replace_particle(char _particle);

    friend class Chronometer;

};








/**
 * @brief CLI argument parsing
 * @param argc number of arguments
 * @param argv arguments
 * @return GameOfLifeParams Game Of Life parameters
 */
GameOfLifeParams parse_cli_args(int argc, char** argv);





struct WindowDimensions {
    int char_rows;
    int char_columns;
};



WindowDimensions get_window_dimensions() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return { w.ws_row, w.ws_col };
}






/**
 * @brief Load a grid from an ascii text file.
 * 
 * @param filepath Grid file to be loaded (.txt)
 * @return Grid Loaded grid
 */
Grid load_grid(const std::string& filepath);

/**
 * @brief Load a grid from a file into a grid of custom dimensions.
 * If the dimensions of the loaded grid are bigger than the requested
 * ones, the loaded grid will be clipped to the requested dimensions.
 * @param filepath Grid file to be loaded (.txt)
 * @param height Requested grid height
 * @param length Requested grid length
 * @param align Loaded grid alignment in requested grid
 * @return Grid Grid to requested dimensions, containing loaded grid
 */
Grid load_grid(const std::string& filepath, int height, int length, GridAlignment align);






/* MAIN */



int main(int argc, char** argv) {

    GameOfLifeParams params = parse_cli_args(argc, argv);

    GameOfLife game_of_life(params);

    game_of_life.start();
    // try {
    // } catch (std::exception e) {
    //     printf("Error: %s\n", e.what());
    // }

}






/* IMPLEMENTATIONS */



Grid::Grid(int height, int length) {
    grid = grid_t(height);
    for(auto& row : grid) {
        row = row_t(length, ' ');
    }
}

Grid::Grid(int height, int length, Grid&& other, GridAlignment align) {
    this->grid = grid_t(height);
    for(auto& row : grid) {
        row = row_t(length, ' ');
    }

    int start_x;
    int start_y;

    switch(align) {
        case center:
            start_x = (this->height() / 2) - (other.height() / 2);
            start_y = (this->width() / 2) - (other.width() / 2);
            break;
        case top_left:
            start_x = 0;
            start_y = 0;
            break;
        case top_right:
            start_x = 0;
            start_y = this->width() - other.width();
            break;
        case bottom_left:
            start_x = this->height() - other.height();
            start_y = 0;
            break;
        case bottom_right:
            start_x = this->height() - other.height();
            start_y = this->width() - other.width();
            break;
        default:
            break;
    }

    for(int i = start_x; i < (start_x + other.height()); i++) {
        for(int j = start_y; j < (start_y + other.width()); j++) {
            if(i < 0 || i >= this->height()) { continue; }
            if(j < 0 || j >= this->width()) { continue; }
            grid[i][j] = other[i - start_x][j - start_y];
        }
    }
}

void Grid::print() {
    for(int i = 0; i < height(); ++i) {
        std::copy(grid[i].begin(), grid[i].end(), std::ostream_iterator<char>(std::cout, " "));
        printf("\n");
    }
    #ifdef MEASURE_TIME
    printf("Avg : %.4fms\n", ms_chrono.avg());
    #endif
}






GameOfLife::GameOfLife(int x, int y, char particle)
        : grid(Grid(x, y)), next_grid(Grid(x, y)) {
            replace_particle(particle);
        }

GameOfLife::GameOfLife(Grid start_grid, char particle)
    : grid(std::move(start_grid)),
    next_grid(Grid(grid.height(), grid.width())) {
        replace_particle(particle);
    }

GameOfLife::GameOfLife(GameOfLifeParams params) 
    : grid(load_grid(params.filepath, params.height, params.width, params.alignment)),
    next_grid(Grid(grid.height(), grid.width())) {
        replace_particle(params.particle);
    }

void GameOfLife::replace_particle(char _particle) {
    this->particle = _particle;
    for(int i = 0; i < grid.height(); ++i) {
        for(char& c : grid[i]) {
            if(c != ' ') { c = particle; }
        }
    }
}


void GameOfLife::start() {
    stale = false;
    #ifdef MEASURE_TIME
    std::thread t_avg(
            [this](){ 
                while(!stale) { 
                    ms_chrono.update_avg(); 
                    std::this_thread::sleep_for(500ms); 
                } 
            });;
    #endif
    while(!stale) {
        display();
        #ifdef MEASURE_TIME
        ms_chrono.start();
        update();
        ms_chrono.stop();
        #else
        update();
        #endif
        #ifdef DEBUG
        printf("Press enter for next step");
        getchar();
        #endif
        std::this_thread::sleep_for(REFRESH_RATE);
    }
    #ifdef MEASURE_TIME
    t_avg.join();
    #endif
}


void GameOfLife::display() {
    system("clear");
    grid.print();
}

void GameOfLife::update() {
    // check();
    check_multithread(NUM_THREADS);
    if(grid == next_grid) { stale = true; }
    else { grid = next_grid; }
}

void GameOfLife::check() {
    int count = 0;
    for(int i = 0; i < grid.height(); ++i) {
        for(int j = 0; j < grid.width(); ++j) {
            count = check_neighbors(i, j);
            #ifdef DEBUG
            printf("%d  %d  : %d neighbors\n", i, j, count);
            #endif
            if(grid[i][j] == particle) {
                if(count < 2 || count > 3) {
                    next_grid[i][j] = ' ';
                    #ifdef DEBUG
                    printf("%d  %d changed from \'*\' to \' \'\n", i, j);
                    #endif
                } else { next_grid[i][j] = particle; }
            } else if(grid[i][j] == ' ' && count == 3) {
                next_grid[i][j] = particle;
                #ifdef DEBUG
                printf("%d  %d changed from \' \' to \'*\'\n", i, j);
                #endif
            } else { next_grid[i][j] = ' '; }
        }
    }
}

void GameOfLife::check_bounded(int height, int width, int start_x, int start_y) {
    int count = 0;
    for(int i = start_x; i < start_x + height; ++i) {
        for(int j = start_y; j < start_y + width; ++j) {
            count = check_neighbors(i, j);
            #ifdef DEBUG
            printf("%d  %d  : %d neighbors\n", i, j, count);
            #endif
            if(grid[i][j] == particle) {
                if(count < 2 || count > 3) {
                    next_grid[i][j] = ' ';
                    #ifdef DEBUG
                    printf("%d  %d changed from \'*\' to \' \'\n", i, j);
                    #endif
                } else { next_grid[i][j] = particle; }
            } else if(grid[i][j] == ' ' && count == 3) {
                next_grid[i][j] = particle;
                #ifdef DEBUG
                printf("%d  %d changed from \' \' to \'*\'\n", i, j);
                #endif
            } else { next_grid[i][j] = ' '; }
        }
    }
}

void GameOfLife::check_multithread(int num_threads) {
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    int step_x = grid.height() / num_threads;
    int rest_x = grid.height() % num_threads;
    // int step_y = num_threads / grid.width();
    // int rest_y = num_threads % grid.width();
    int step_y = grid.width();
    for(int i = 0; i < num_threads; i++) {
        if(i != num_threads - 1) {
        threads.emplace_back(
            std::thread(&GameOfLife::check_bounded, 
                        this, 
                        step_x, 
                        step_y,
                        i*step_x, 
                        0));
        } else {
            threads.emplace_back(
                std::thread(&GameOfLife::check_bounded, 
                            this, 
                            step_x + rest_x, 
                            step_y, 
                            i*step_x, 
                            0));
        }
    }
    for(auto& thread : threads) {
        thread.join();
    }
}

int GameOfLife::check_neighbors(int x, int y) {
    int count = 0;
    for(int i = (x - 1); i <= (x + 1); i++) {
        if(i < 0 || i >= grid.height()) { continue; }
        for(int j = (y - 1); j <= (y + 1); j++) {
            if(j < 0 || i >= grid.width()) { continue; }
            if(i != x || j != y) {
                if(grid[i][j] == particle) { count++; }
            }
        }
    }
    return count;
}






GameOfLifeParams parse_cli_args(int argc, char** argv) {
    
    GameOfLifeParams params {};
    const char* exe = argv[0];
    opterr = 0;
    int c;

    if(argc < 2) { usage(exe); exit(1); }
    while((c = getopt(argc, argv, "p:a:")) != -1) {
        switch (c) {
            case 'p':
                params.particle = optarg[0];
                break;
            case 'a':
                if(strcmp(optarg, "center") == 0)          { params.alignment = center; }
                if(strcmp(optarg, "top-left") == 0)        { params.alignment = top_left; }
                if(strcmp(optarg, "top-right") == 0)       { params.alignment = top_right; }
                if(strcmp(optarg, "bottom-left") == 0)     { params.alignment = bottom_left; }
                if(strcmp(optarg, "bottom-right") == 0)    { params.alignment = bottom_right; }
                break;
            case '?':
                switch(optopt) {
                    case 'p': case 'a':
                        printf("Option -%c requires an argument\n", optopt);
                        break;
                    default:
                        if(isprint(optopt)) {
                            printf("Invalid option: -%c\n", optopt);
                        }
                        break;
                }
                exit(1);
                break;
            default:
                exit(1);
        }
    }
    if((optind - argc) > 1) {
        usage(exe);
        exit(1);
    }
    params.filepath = argv[optind];

    auto window_size = get_window_dimensions();
    int size = window_size.char_rows < window_size.char_columns ? window_size.char_rows : window_size.char_columns;
    params.height = size;
    params.width = size;

    return params;
}






Grid load_grid(const std::string& filepath) {
    std::fstream fs(filepath);
    if(!fs.is_open()) {
        std::cerr << "Error: " << filepath << " not found" << std::endl;
        exit(1);
    }
    std::string line;

    grid_t grid;

    while(getline(fs, line)) {
        row_t row;
        std::copy(line.begin(), line.end(), std::back_inserter(row));
        grid.push_back(row);
    }

    /**
     * This part expands every row to the length 
     * of the longest one, to ensure a rectangular grid
     */

    int max_length = (std::max(grid.begin(), 
                                grid.end(), 
                                [](auto& lhs, auto& rhs){ 
                                    return (*lhs).size() > (*rhs).size(); 
                                }))->size();

    for(auto& row : grid) {
        if(row.size() < max_length) { 
            row.insert(row.end(), (row.size() - max_length), ' ');
        }
    }

    return { grid };
}



Grid load_grid(const std::string& filepath, int height, int length, GridAlignment align) {
    return { height, length, load_grid(filepath), align };
}