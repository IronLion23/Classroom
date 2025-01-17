#include <cstdlib>
#include <ctime>
#include <iostream>
#include <csignal>
#include <string>
#include <cstring>

#define func(n,a) void show##n() { std::cout << a;}
#define func(n,a) void show##n() { std::cout << a;}
func(Title, "**************************************\n*        Paper Rock Scissors         *\n**************************************\n\n")
func(Menu, "Enter rock, paper or scissors: ")
#undef func

#define func(n, a) void ask##n() { std::cout << a;}
func(PlayAgain, "\nWould you like to play again? [y or n]: ")
func(SeeScore, "Would you like to see the score? [y or n]: ")//unused function currently
#undef func

/*scoreboard not fully implemented yet*/
sig_atomic_t computerScore = 0;
sig_atomic_t userScore = 0;
sig_atomic_t drawCount = 0;

#define func(n, a, b) void declare##n(int param){std::cout << a <<"\n"; ++b;}
func(CompWinner, "Computer Wins!", computerScore)
func(UserWinner, "User Wins!", userScore)
func(ADraw, "Draw!", drawCount)
#undef func

#define func(n, a, b, c) void score##n(){std::cout << "\nWins | Draws | Losses |\n  " << a << "  |   " << b << "   |   " << c << "    | \n";}
func(Board, userScore, drawCount, computerScore)

auto getWinnerMessage = [] (std::string winnerChoice){if(winnerChoice =="paper")std::cout << "Paper covers Rock...";
                                 else if(winnerChoice == "rock")std::cout << "Rock crushes Scissors...";
                                 else
                                 {std::cout << "Scissors cuts Paper...";}};

using namespace std;
 
char opts[] = {'n', 'y'};
string choice[] = {"rock", "paper", "scissors"
, "paper","rock", "scissors"
, "scissors", "rock", "paper"
, "scissors", "paper", "rock"
, "rock", "scissors", "paper"
, "paper", "scissors", "rock"};
enum {rock, paper, scissors};
enum {n, y};
 
 
int main()
{
    bool playAgain = true;
    char userWantsToPlayAgain;
    void (*result_handler)(int);
    showTitle();
    do
    {
        showMenu();
        string userChoice;
        cin >> userChoice;
        srand(time(NULL));
        string computerChoice = choice[rand() % 18];
        cout << "\nUser: "<< userChoice <<"\nComputer: " << computerChoice << "\n";
 
        if((strcmp(userChoice.c_str(), choice[rock].c_str()) == 0) && (strcmp(computerChoice.c_str(), choice[paper].c_str()) == 0) 
            || (strcmp(userChoice.c_str(), choice[paper].c_str()) == 0) && (strcmp(computerChoice.c_str(), choice[scissors].c_str()) == 0)
            || (strcmp(userChoice.c_str(), choice[scissors].c_str()) == 0) && (strcmp(computerChoice.c_str(), choice[rock].c_str()) == 0))
        {
            result_handler = signal(SIGINT, declareCompWinner);
            getWinnerMessage(computerChoice);
        }
        else if(strcmp(userChoice.c_str(), computerChoice.c_str()) == 0)
            result_handler = signal(SIGINT, declareADraw);
        else
        {
            result_handler = signal(SIGINT, declareUserWinner);
            getWinnerMessage(userChoice);
        } 
        raise(SIGINT);
 
        askPlayAgain();
        std::cin >> userWantsToPlayAgain;
        playAgain = (userWantsToPlayAgain == opts[n]) ? false : true;
    }while(playAgain);
 
    scoreBoard();
 
    return 0;
}