// Patrick Harris - Program 1
// gcc -o source source.c -lpthread             compile
// ./source                                     run source

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>


// 52 card deck
int deckOfCards[52];
int sizeOfCards;

//winner checker and player mover tracker
int winner, playerMove;

//pointer to log file
FILE *file;

// initialize struct for card tracing
typedef struct players {
	int id;
	int cardOne;
	int cardTwo;
} player_n;

//setting up players 1-3 from struct players
player_n playerOne;
player_n playerTwo;
player_n playerThree;

//thread mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


//semaphore to halt players for dealer and for each player to take their turn
sem_t playerOneS;
sem_t playerTwoS;
sem_t playerThreeS;

//function to pickCard
int pickCard();

//place card to the bottom of the deck
void dismissCard(int card);

//display the card deck
void displayDeck();

//write to the log file
void writeToFile();

//function for the dealer to shuffles the cards
void *dealerShuffle(void *args);

//function to run the 3 players for the program
void *playersPlay(void *args);

//main
int main(int argc, char *argv[]) {
	int countRounds = 0;
	int x, y;

	pthread_t thread_playerOne;
	pthread_t thread_playerTwo;
	pthread_t thread_playerThree;
	pthread_t thread_dealer;

	srand((unsigned)time(NULL));

	//call the deck, make sure there are 13 cards per the 4 suits
	sizeOfCards = 0;

	for(x = 0; x < 4; x++)
	{
		for(y = 1; y <= 13; y++)
        {
			deckOfCards[sizeOfCards++] = y;
		}
	}

	//creating player 1
	playerOne.id = 1;
	playerOne.cardOne = -1;
	playerOne.cardTwo = -1;

	//creating player 2
	playerTwo.id = 2;
	playerTwo.cardOne = -1;
	playerTwo.cardTwo = -1;

	//creating player 3
	playerThree.id = 3;
	playerThree.cardOne = -1;
	playerThree.cardTwo = -1;

	//open the log file for program tracing
	file = fopen("log.txt", "w");

	//the for loop counter will stop after 3 rounds
	for(countRounds = 0; countRounds < 3; countRounds++)
    {
        //each round new player starts
        playerMove = countRounds + 1;
		fprintf(file, "ROUND %d\n", (countRounds+1));
		printf("ROUND %d\n", (countRounds+1));

		//semaphores that will halt the players for the dealer
		sem_init(&playerOneS, 0, 0);
		sem_init(&playerTwoS, 0, 0);
		sem_init(&playerThreeS, 0, 0);

		//begins the thread for the players
		pthread_create(&thread_playerOne, NULL, &playersPlay, &playerOne);
		pthread_create(&thread_playerTwo, NULL, &playersPlay, &playerTwo);
		pthread_create(&thread_playerThree, NULL, &playersPlay, &playerThree);

		//begins the thread for the dealer
		pthread_create(&thread_dealer, NULL, &dealerShuffle, NULL);

    	//wait till the current round is finished before moving on
    	pthread_join(thread_playerOne, NULL);
    	pthread_join(thread_playerTwo, NULL);
    	pthread_join(thread_playerThree, NULL);
    	pthread_join(thread_dealer, NULL);

		fprintf(file, "\n");
		printf("\n");
	}
	fclose(file); //close the log file
	return 0;
}

int pickCard()
{
	int x, card;
	card = deckOfCards[0];

	for(x = 0; x < sizeOfCards - 1; x++)
		deckOfCards[x] = deckOfCards[x+1];

	sizeOfCards--;
	return card;
}

void dismissCard(int card)
{
	deckOfCards[sizeOfCards++] = card;
}

void displayDeck() {
	int x;
	printf("DECK: ");

	for(x = 0; x < sizeOfCards; x++)
    {
		printf("%d ",deckOfCards[x]);
	}
	printf("\n");
}

void writeToFile()
{
	int x;
	fprintf(file, "DECK: ");

	for(x = 0; x < sizeOfCards; x++)
    {
		fprintf(file, "%d ", deckOfCards[x]);
	}

	fprintf(file, "\n");
}

void *dealerShuffle(void *args) {
	int x, y, z;

	//dealer begins the card shuffle
	fprintf(file, "Dealer: shuffle \n");

	//dealer reshuffle
	for(x = 0; x < 52; x++)
    {
		y = x + rand() / (RAND_MAX / (52 - x) + 1);
		z = deckOfCards[x];
		deckOfCards[x] = deckOfCards[y];
		deckOfCards[y] = z;
	}
	sizeOfCards = 52;
	winner = -1;
	displayDeck();

	//players pick a card
	playerOne.cardOne = pickCard();
	playerTwo.cardOne = pickCard();
	playerThree.cardOne = pickCard();

	//players begin to play
	//player semaphores post are used to release in order to
	//make the play
	// 1 waits for 3, 2:1, and 3:2
	if(playerMove == 1)
    {
        sem_post(&playerThreeS);
    } else if(playerMove == 2) {
        sem_post(&playerOneS);
    } else if(playerMove == 3) {
        sem_post(&playerTwoS);
    }

	return (void *)NULL;
}

void *playersPlay(void *args) {
	player_n *players;
	int win;
	players = (player_n *)(args);
	win = -1;


	//players play until round is over
	while(1)
    {
        //player semaphore wait are used to
        //make players wait for the other to
        //make a move
        if(players->id ==1)
        {
            sem_wait(&playerThreeS);
        } else if (players->id ==2){
            sem_wait(&playerOneS);
        } else if (players->id ==3){
            sem_wait(&playerTwoS);
        }

		pthread_mutex_lock(&mutex);

		if(winner != -1) { //breaks if a player wins
			pthread_mutex_unlock(&mutex);
			break;
		}

        //log the hand of the player
		if(players->cardOne != -1)
        {
			fprintf(file, "PLAYER %d: hand %d\n", players->id, players->cardOne);
		} else
		{
			fprintf(file, "PLAYER %d: hand %d\n", players->id, players->cardTwo);
		}


		//pick new card and log that the card that is drawn
		if(players->cardOne == -1) {
			players->cardOne = pickCard();
			fprintf(file, "PLAYER %d: draws %d\n", players->id, players->cardOne);
		} else {
			players->cardTwo = pickCard();
			fprintf(file, "PLAYER %d: draws %d\n", players->id, players->cardTwo);
		}

		//print the hand (2 cards) of the player
		fprintf(file, "PLAYER %d: hand %d %d\n", players->id, players->cardOne, players->cardTwo);

		printf("PLAYER %d: hand %d %d\n", players->id, players->cardOne, players->cardTwo);

		//check to see if there is a winner
		if(players->cardOne == players->cardTwo) {
			//force break if there is a winner
			printf("PLAYER %d WIN: yes\n", players->id);
			winner = players->id;
			pthread_mutex_unlock(&mutex);

			//make players stop waiting at the end of the round
            sem_post(&playerOneS);
            sem_post(&playerTwoS);
            sem_post(&playerThreeS);

            break;

		}

		printf("PLAYER %d WIN: no\n", players->id);

		//if there is no winner then dismiss 1 of the players cards at random
		if(rand() % 2 == 0) {
			fprintf(file, "PLAYER %d: discards %d\n", players->id, players->cardOne);
			dismissCard(players->cardOne);
			players->cardOne = -1;
		} else {
			fprintf(file, "PLAYER %d: discards %d\n", players->id, players->cardTwo);
			dismissCard(players->cardTwo);
			players->cardTwo = -1;
		}

		//print the final hand of the players in the round
		if(players->cardOne != -1) {
			fprintf(file, "PLAYER %d: hand %d\n", players->id, players->cardOne);
		} else {
			fprintf(file, "PLAYER %d: hand %d\n", players->id, players->cardTwo);
		}

		//display the final deck of the round
		displayDeck();
		writeToFile();
		pthread_mutex_unlock(&mutex);

		//enter the next thread
		//players take turns playing in rounds
		if(players->id ==1)
        {
            sem_post(&playerTwoS);
        } else if (players->id ==2){
            sem_post(&playerThreeS);
        } else if (players->id ==3){
            sem_post(&playerOneS);
        }
	}

	//players will begin to exit the current round
	if(winner == players->id) {
		fprintf(file, "PLAYER %d: wins and exits round\n", players->id);
	} else {
		fprintf(file, "PLAYER %d: exits round\n", players->id);
	}
	return (void *)NULL;
}
