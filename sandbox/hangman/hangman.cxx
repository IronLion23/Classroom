#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <typeinfo>

#include "hangman.hxx"

void (*result_handler)(int);
typedef void (*getResultFunc)(int);

void endOfRoundMessage(getResultFunc result)
{
    result_handler = signal(SIGINT, result);
}

char response[] = {'n', 'y', 's'};
enum {n, y};
using namespace std;

int main() 
{
    word guess;
    string line;
    bool playAgain = false;
    bool userWonRound = false, survivorModeEnabled = false;
    char userResponse[1];
    char userGuess[1];
    int getWordAtLocation = 0, correctOrSameGuessCounter = 0, userSubMenuResponseI = 0, streak = 0;
    guess.setUsersMaxStreakOfAllTime();

    showTitle();

    gameModeMenu:
    showMenu();
    string userGameModeMenuResponse_str;
    cout << "User: "; cin >> userGameModeMenuResponse_str;

    while(!isdigit(userGameModeMenuResponse_str[0]))
    {
        cout << "\nYou must enter a valid menu option (1-2)\n";
        showMenu(); cout << "User: "; cin >> userGameModeMenuResponse_str;
    }

    int userGameModeMenuResponse_I = 1;
    userGameModeMenuResponse_I = stoi(userGameModeMenuResponse_str);
    switch(userGameModeMenuResponse_I)
    {
        case 1:
                survivorModeEnabled = true;
                break;
        case 2:
                survivorModeEnabled = false;
                guess.resetTries();
                correctOrSameGuessCounter = 0;
                userWonRound = false;
                guess.resetFirstGuessWasCorrectValue();
                break;
        case 3: 
                goto quit;
        default:
                cout << "\nInvalid entry!\n";
                goto gameModeMenu;
    }

    do
    { 
        int count = 1;
        srand(time(NULL));

        getWordAtLocation = rand() % 173139;
        ifstream dictionaryFile ("dictionary.txt");

        if(dictionaryFile.is_open())
        {
            while ( getline (dictionaryFile,line) && count != getWordAtLocation)
            {	
        	   count++;
            }
            guess.setWord(line);
            dictionaryFile.close();
        }

        do{


            main_menu:

            if(guess.getTriesLeft() == 0)
            {
                goto outOfTries;
            }

            const char * userGuessWord;
            bool guessSingleChar = true, userGuessedRightWord = false;

            showSubMenu();
            string userSubMenuResponse_str;
            cout << "User: "; cin >> userSubMenuResponse_str;

            while(!isdigit(userSubMenuResponse_str[0]))
            {
                cout << "\nYou must enter a valid menu option (1-7)\n";
                showSubMenu(); cout << "User: "; cin >> userSubMenuResponse_str;
            }

            userSubMenuResponseI = stoi(userSubMenuResponse_str);

            switch(userSubMenuResponseI)
            {
                case 1:
                {
                    caseOneStart:
                    string userGuess_str;
                    int userGuess_i = 1;

                    askForLetter();
                    cout << "\n";
                    guess.displayHangMan();
                    cout << "Word: " << guess.incompleteWord() <<  "\n";

                    lineWrapper(string("\nGuessed letters: {" + guess.getGuessedLetters() +  "}"), '~');
                    cout << "Guessed letters: {" << guess.getGuessedLetters() <<  "}\n";
                    lineWrapper(string("\nGuessed letters: {" + guess.getGuessedLetters() +  "}"), '~');

                    cout << "U: "; cin >> userGuess_str;
                    if(isdigit(userGuess_str[0]))
                    {
                        userGuess_i = stoi(userGuess_str);
                        if(userGuess_i == 1)
                            goto main_menu;
                        else
                            goto caseOneStart;
                    }
                    else if(userGuess_str.size() > 1)
                    {
                        cout << "\n**You entered more than one character...please enter only one letter**\n";
                        goto caseOneStart;
                    }
                    else
                    {
                        userGuess[0] = userGuess_str[0];
                    }
                    break;
                }
                case 2:
                {
                    caseTwoStart:
                    guessSingleChar = false;
                    lineWrapper(string("\nUnfinished word: " + string(guess.showWord()) + "\n"), '*');
                    cout << "Unfinished word: " << guess.showWord() << "\n";

                    lineWrapper(string("\nGuessed letters: {" + guess.getGuessedLetters() +  "}"), '-');
                    cout << "Guessed letters: {" << guess.getGuessedLetters() << "}\n";
                    lineWrapper(string("\nGuessed letters: {" + guess.getGuessedLetters() +  "}"), '*');
                    cout << "\n";

                    askForSuspectedWord();
                    cin.ignore(); cout << "Enter the full word (Enter 1 to go back to main menu): ";
                    string userGuessWord_str;
                    cin >> userGuessWord_str;
                    if(isdigit(userGuessWord_str[0]))
                    {
                        int userGuessWord_i = stoi(userGuessWord_str);
                        if(userGuessWord_i == 1)
                            goto main_menu;
                        else
                            goto caseTwoStart;
                    }
                    else if(ispunct(userGuessWord_str[0]))
                    {
                        cout <<  "\n**You entered something that doesn't make sense...**\n";
                        goto caseTwoStart;
                    }
                    else
                        //cin.getline(userGuessWord, 256);
                        userGuessWord = userGuessWord_str.c_str();
                    for(auto c : userGuessWord_str)
                    {
                        bool alreadyGuessed = false;
                        if(!isalpha(c))
                          continue;
                        for(auto w : string(guess.getGuessedLetters()))
                        {
                            if(w == c)
                            {
                                alreadyGuessed = true;
                                break;
                            }
                        }
                        if(alreadyGuessed)
                        {
                            continue;
                        }
                        else if(!guess.guessLetter(c))
                        {
                            guess.subtractTry();
                        }
                        if(guess.getTriesLeft() == 0)
                        {
                          break;
                        }
                    }
                    if(guess.showWord() == guess.getWord() && userGuessWord == guess.getWord())
                    {
                        userWonRound = true;
                        userGuessedRightWord = true;
                        endOfRoundMessage(declareUserWinsRound);
                        goto userDiscoveredTheWord;
                    }
                    if(guess.getTriesLeft() != 0 && guess.showWord() != guess.getWord())
                    {
                        cout << "\nYou didn't get the word correct, but you got some letters correct...keep going!\n";
                        cout << "Unfinished word: " ;
                        for(auto c : string(guess.showWord()))
                            cout << c << " ";
                        cout << "\n"; 
                        goto main_menu;
                    }
                    else if(guess.getTriesLeft() == 0)
                        break;
                }
                case 3:
                {
                    cout << "\n";
                    lineWrapper(string("\nGuessed letters: {" + guess.getGuessedLetters() +  "}"), '~');
                    cout << "Guessed letters: {" << guess.getGuessedLetters() <<  "}\n";
                    lineWrapper(string("\nGuessed letters: {" + guess.getGuessedLetters() +  "}"), '~');
                }
                case 4:
                {
                    lineWrapper(string("Incomplete Word: " + string(guess.incompleteWord()) + "\n"), '~');
                    cout << "Incomplete Word: ";
                    for(auto c : string(guess.showWord()))
                        cout << c << " ";
                    cout << "\n"; 
                    lineWrapper(string("Incomplete Word: " + string(guess.incompleteWord()) + "\n"), '~');
                }
                case 5:
                {
                    lineWrapper(string("Tries left: " + to_string(guess.getTriesLeft()) + "\n"), '~');
                    cout << "Tries left: " << guess.getTriesLeft() << "\n";
                    lineWrapper(string("Tries left: " + to_string(guess.getTriesLeft()) + "\n"), '~');
                    cout << "\n";
                    goto main_menu;    
                }
                case 6:
                {
                    guess.displayHangMan();
                    cout << "\n";
                    goto main_menu;
                }
                case 7:
                {
                    goto quit;
                }
                default:
                {
                    cout << "\nYou must enter a valid option (1-7)\n";
                    goto main_menu;
                }
            }

            if(guess.getTriesLeft() == 0 || userWonRound)
                break;

            bool alreadyGuessed = false;

            for(auto c : guess.getGuessedLetters())
                if(userGuess[0] == c)
                    alreadyGuessed = true;

            if(!guess.guessLetter(userGuess[0]) && guessSingleChar)
            {
                guess.subtractTry();
                guess.displayHangMan();
                lineWrapper(string("\nGuessed letters: {" + guess.getGuessedLetters() +  "}"), '*');
                cout << "Guessed letters: {" << guess.getGuessedLetters() <<  "}\n";
                lineWrapper(string("\nGuessed letters: {" + guess.getGuessedLetters() +  "}"), '*');
                streak = 0;
            }
            else
            {
                if(!alreadyGuessed)
                    streak++;
                
                if(streak > guess.getMaxStreak())
                    guess.setMaxStreak(streak);
                guess.displayHangMan();
            }

            std::cout << "\nWord: " ;

            for(auto c : string(guess.showWord()))
                cout << c << " ";

            cout << "\n";
            cout << "Streak: " << streak << "\n";

            if(guess.showWord() == guess.getWord())
            {
                userWonRound = true;
                endOfRoundMessage(declareUserWinsRound);
                if(guess.checkIfFirstGuessWasCorrect())
                    guess.incrementFirstGuessToWonRoundConversionTracker();
            }
        }while(guess.getTriesLeft() != 0 && !userWonRound);

        outOfTries:

        if(guess.getTriesLeft() == 0)
        {
            endOfRoundMessage(declareOutOfGuesses);
        }

        userDiscoveredTheWord:

        raise(SIGINT);

        cout << guess.getWord() << "\n";

        endOfRoundMenu:

        if(survivorModeEnabled)
        {
            if(userWonRound)
                userResponse[0] = 'y';
            else
                userResponse[0] = 'n';
        }
        else
        {
            askPlayAgain();
            cin >> userResponse;
            if(isupper(userResponse[0]))
                userResponse[0] = tolower(userResponse[0]);

            while(!isalpha(userResponse[0]) || (userResponse[0] != 'n' && userResponse[0] != 'y' && userResponse[0] != 's') || (string(userResponse).size() > 1))
            {
                if(string(userResponse).size() > 1)
                    cout << "You entered more than one letter...please follow instructions.\n";
                else
                    cout << "You did not enter a valid response!\n";
                askPlayAgain();
                cin >> userResponse;
                if(isupper(userResponse[0]))
                    userResponse[0] = tolower(userResponse[0]);
            }
        }

        switch(userResponse[0])
        {
            case 'n':
                    playAgain = false;
                    break;
            case 'y':
                    playAgain = true;
                    break;
            case 's': 
                    guess.getStats();
                    goto endOfRoundMenu;
            default: 
                    cout << "\nInvalid entry!\n";
                    goto endOfRoundMenu;
        }
        if(playAgain)
        {
            guess.resetTries();
            correctOrSameGuessCounter = 0;
            userWonRound = false;
            guess.resetFirstGuessWasCorrectValue();
        }
    }while(playAgain);

    //scoreBoard();
    if(survivorModeEnabled)
    {
        guess.getStats();
        goto gameModeMenu;
    }
    else 
        scoreBoard();


    quit:
    
    return 0;
}
