#include <time.h>
#include "LCD.h"
#include "portyLcd.h"
#include <string.h>
#include "notes.h" // co to ??
#include "msp430x14x.h" // TODO moze zmienic trzeba bedzie "" na <>

#include <time.h> // TODO chyba potrzebne ?
#include <stdlib.h> // TODO chyba potrzebne ?


//---------------- zmienne globalne -------------
unsigned int counter = 0; // zmienna do opóźniania działania przycisków
uint8 pos = 0; // zmienna wyznaczająca szybkość zająca TODO zmienic
uint8 option = 0; // zmienna wyboru opcji TODO zmienic
int highscorePointsTab[3] = {0, 0, 0}; // 1, 2, 3 miejsce
char highscoreNicksTab[3][3] = {{' ', ' ', ' '},
                                {' ', ' ', ' '},
                                {' ', ' ', ' '}}; // 1, 2, 3 miejsce

//--------------------------------

char avatar[8] = {0x03, 0x07, 0x0E, 0x1C, 0x1C, 0x0E, 0x07, 0x03}; // lapki
char point1[8] = {0x00, 0x00, 0x00, 0x06, 0x06, 0x00, 0x00, 0x00}; // kwadracik
char point2[8] = {0x00, 0x00, 0x06, 0x0F, 0x0F, 0x06, 0x00, 0x00}; // koleczko
char point3[8] = {0x00, 0x00, 0x00, 0x04, 0x0E, 0x1F, 0x00, 0x00}; // trojkacik
//-------------------------------------------

// Naglowki funkcji

void menu(void); // glowne menu gry

void game(void); // kod gry

void authors(void); // autorzy

void highScore(void); // tablica najlepszych wynikow

void saveHighScore(int); // zapisanie nicku i wyniku gracza

void levelScreen(int); // ekran poziomu

//void initChars(void); // inicjacja nowych znakow

void writeText(unsigned char *); // wypisanie na ekran ciagu znakow

void writeNumber(int); // wypisanie na ekran liczby

int getRandomNumber(int, int); // losowanie liczy w przedziale

void createChars(char *, int); // zapisanie wygenerowanych znakow w pamieci
//-------------------------------------------


void main(void) {
    WDTCTL = WDTPW + WDTHOLD; // wyłączenie mechanizmu Watchdog
    P2DIR |= BIT1; // ustawienie bitu P2.1 (dioda status) na 1 (tryb wyjściowy)
    P4DIR |= 0x0C; // ustawienie bitu P4.2, P4.3 (buzzer) na 1 (tryb wyjściowy)
    P4DIR &= 0x0F; // ustawienie bitu P4.4, P4.5, P4.6, P4.7 (buttony) na 0 (tryb wejściowy)
    InitPortsLcd(); // inicjalizacja portów LCD
    InitLCD(); // inicjalizacja LCD
    clearDisplay(); // czyszczenie wyświetlacza
//    initChars(); // inicjalizacja naszych znakow

    /* ustawienie Basic Clock Module na ACLK (zegar 8MHz) i podział częstotliwości przez 1 (8MHz) */
    BCSCTL1 |= XTS; // ACLK = LFXT1 = HF XTAL 8MHz
    do {
        IFG1 &= ~OFIFG; // czyszczenie flgi OSCFault
        for (unsigned int i = 0xFF; i > 0; i--); // odczekanie
    } while ((IFG1 & OFIFG) == OFIFG); // dopóki OSCFault jest ciągle ustawiona
    BCSCTL1 |= DIVA_0; // ACLK = 8MHz / 1 = 8MHz
    BCSCTL2 |= SELM0 | SELM1; // MCLK = LFTX1 = ACLK

    /* ustawienie Timer_A na ...kHz, a przerwanie co ...ms */
    TACTL = TASSEL_1 + MC_1 + ID_0; // wybór ACLK, ACLK / ... = ...kHz, tryb Up
    CCTL0 = CCIE; // włączenie przerwań od CCR0
    CCR0 = 40000; // podzielnik ...: przerwanie co ...ms
    _EINT(); // włączenie przerwań
    for (;;) {
        _BIS_SR(LPM3_bits); // przejście do trybu LPM3
        menu(); // przejście do menu
        counter++; // inkrementacja countera
        counter = counter % 30; // reset countera co 30
    }
}


// procedura obsługi przerwania od Timer A
#pragma vector = TIMERA0_VECTOR

__interrupt void Timer_A(void) {
    _BIC_SR_IRQ(LPM3_bits); // wyjście z trybu LPM3
    pos++; // zmienna inkrementowana zegarem A służaca do wyznaczania szybkości zająca
    pos = pos % 100; // ograniczenie zmiennej
}




void menu(void) {
    switch (option) { // przesuwanie wskaźnika wyboru zależne od zmiennej option
        case 0:
            SEND_CMD(DD_RAM_ADDR);
            writeText(" > GAME < ");
            SEND_CMD(DD_RAM_ADDR2);
            writeText("AUTHORS");
            break;
        case 1:
            SEND_CMD(DD_RAM_ADDR);
            writeText("GAME");
            SEND_CMD(DD_RAM_ADDR2);
            writeText(" > AUTHORS < ");
            break;
        case 2:
            SEND_CMD(DD_RAM_ADDR);
            writeText("AUTHORS");
            SEND_CMD(DD_RAM_ADDR2);
            writeText(" > HIGHSCORE < ");
            break;
    }
    if ((P4IN & BIT7) == 0) { // odczytanie stanu bitu P4.7 (jeśli przycisk jest wciśnięty)
        clearDisplay();
        switch (option) { // włączenie wybranej opcji
            case 0: // gra
                levelScreen(1);
                game();
                option = 0;
                break;
            case 1: // autorzy
                authors();
                option = 1;
                break;
            case 2: // najlepsi zawodnicy
                highScore();
                option = 2;
                break;
        } // TODO co robi to ponizej?
    } else if ((P4IN & BIT4) == 0) { // odczytanie stanu bitu P4.4 (jeśli przycisk jest wciśnięty)
        if (counter % 5 == 0) {
            counter = 0;
            if (option != 0)
                option--; // dekrementacja zmiennej option - przesunięcie wskaźnika wyboru w górę
        }
    } else if ((P4IN & BIT5) == 0) {
        if (counter % 5 == 0) {
            counter = 0;
            if (option != 3)
                option++; // inkrementacja zmiennej option - przesunięcie wskaźnika wyboru w dół
        }
    }
}

void game(void) {
    // TODO
    P2OUT = P2OUT | BIT1; // ustawienie bitu P2.1 na stan wysoki (dioda status gaśnie)
    // Ustawienia gry TODO wszystko ma byc gdy guzik jest puszczony by lepiej dzialalo
    int playerPosition = 1; // pozycja gracza
    int boostPosition = 12; // pozycja punktu do zebrania
    int rate = 60; // tempo
    int level = 1; // poziom
    int points = 0; // punkty
    int totalPoints = 0; // suma punktow
    int life = 3; // zycia - traci jak nie zlapie punktu
//    int buttonPressed = 0; // wcisniecie przycisku

    while (1) {
        SEND_CHAR(' ');
        createChars(avatar, 1);
        createChars(point1, 2);
        createChars(point2, 3);
        createChars(point3, 4);










        if ((P4IN & BIT6) == 0) { // jesli przycisk do lapania wcisniety sprawdzenie czy gracz zlapal
            while ((P4IN & BIT6) == 0) { // dopoki nie pusci przycisku TODO moze bedzie trzeba dodac  warunek && buttonPressed = 0
                // w srodku chyba nic nie trzeba TODO sprawdzic
            }
            P1OUT = P1OUT | BIT5; // ustawienie bitu P2.1 na stan niski (dioda status świeci)
//            buttonPressed = 1;
            clearDisplay();
            if (boostPosition == playerPosition + 1) { // jesli punkt jest kratke przed postacia
                level++;
                levelScreen(level);
                if (rate != 0) { // zmniejszenie rate wplywa na zwiekszenie tempa gry
                    rate -= 2;
                }
                totalPoints += points;
            }


            // TODO
        }
    }
}

void authors() {
// przewijani w dół autorzy gry
    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("Mateusz");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("Horczak");
    for (long i = 0; i < 3000000; i++);

    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("Konrad");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("Jankowski");
    for (long i = 0; i < 3000000; i++);

    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("Szymon");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("Lupinski");
    for (long i = 0; i < 3000000; i++);
}


void highScore(void) {
    // przewijana w dół lista 3 najlepszych graczy wraz z punktacją

    SEND_CMD(DD_RAM_ADDR);
    writeText("1. ");
    writeNumber(highscorePointsTab[0]); // wyświetlenie punktacji
    writeText("   ");
    for (int i = 0; i < 3; i++) // wyświetlenie nicku
        SEND_CHAR(highscoreNicksTab[0][i]);

    SEND_CMD(DD_RAM_ADDR2);
    writeText("2. ");
    writeNumber(highscorePointsTab[1]);
    writeText("   ");
    for (int i = 0; i < 3; i++)
        SEND_CHAR(highscoreNicksTab[1][i]);

    for (long i = 0; i < 3000000; i++);
    clearDisplay();

    SEND_CMD(DD_RAM_ADDR);
    writeText("2. ");
    writeNumber(highscorePointsTab[1]);
    writeText("   ");
    for (int i = 0; i < 3; i++)
        SEND_CHAR(highscoreNicksTab[1][i]);

    SEND_CMD(DD_RAM_ADDR2);
    writeText("3. ");
    writeNumber(highscorePointsTab[2]);
    writeText("   ");
    for (int i = 0; i < 3; i++)
        SEND_CHAR(highscoreNicksTab[2][i]);

    for (long i = 0; i < 3000000; i++);
}


void writeText(unsigned char *text) {
    for (int i = 0; i < strlen(text); i++) // wypisywanie po kolei znaków z tablicy
        SEND_CHAR(text[i]);
}


void writeNumber(int x) {
    if (x >= 10)
        writeNumber(x / 10);
    int number = x % 10 + 48; // zamiana liczby na znak ASCII
    SEND_CHAR(number);
}


void levelScreen(int level) {
    writeText("LEVEL "); // wypisanie numeru poziomu
    writeNumber(level);
    for (long i = 0; i < 10000; i++);
}


void saveHighScore(int points) {
    int letter = 0; // wybierana litera
    char nick[3] = {'_', '_', '_'}; // 3 literowy nick gracza
    int curr = 0;

    counter = 0;

    while (1) {
        counter++;
        counter = counter % 30;

        SEND_CMD(DD_RAM_ADDR + 6);
        writeNumber(points); // wyświetlenie zdobytych punktów
        SEND_CMD(DD_RAM_ADDR2 + 6);

        // pola zapisu nicku
        SEND_CHAR(nick[0]);
        SEND_CHAR(' ');
        SEND_CHAR(nick[1]);
        SEND_CHAR(' ');
        SEND_CHAR(nick[2]);
        if ((P4IN & BIT7) == 0 && nick[curr] != '_') { // przycisk P4.7 -> zapis obecnej litery i przejście do kolejnej
            letter = 0;
            curr++;

            if (curr == 3) { // jeśli została zapisana ostatnia litera -> wyjdź z pętli
                SEND_CMD(DD_RAM_ADDR2 + 6);
                writeText("SAVED");

                for (long i = 0; i < 100000; i++); // odczekanie

                SEND_CMD(DD_RAM_ADDR2 + 6);
                SEND_CHAR(nick[0]);
                SEND_CHAR(' ');
                SEND_CHAR(nick[1]);
                SEND_CHAR(' ');
                SEND_CHAR(nick[2]);
                break;
            }
        } else if ((P4IN & BIT4) == 0) { // odczytanie stanu bitu P4.4 (jeśli przycisk jest wciśnięty)
            if (counter % 10 == 0) {
                counter = 0;
                if (letter != 0) {
                    letter--; // dekrementacja zmiennej letter - zmiana litery
                }
                nick[curr] = letter + 65; // zmiana liczby na literę w ASCII
            }
        } else if ((P4IN & BIT5) == 0) { // odczytanie stanu bitu P4.5 (jeśli przycisk jest wciśnięty)
            if (counter % 10 == 0) {
                counter = 0;
                if (letter < 25)
                    letter++; // inkrementacja zmiennej letter - zmiana litery
                nick[curr] = letter + 65; // zmiana liczby na literę w ASCII
            }
        }
    }
    for (long i = 0; i < 10000; i++); // odczekanie

    // zapis rekordu z punktacją i nickiem w odpowiednim miejscu na liście
    int place = -1;
    for (int i = 0; i < 3; i++) {
        if (points >= highscorePointsTab[i]) {
            place = i;
            break;
        }
    }
    if (place != -1) {
        for (int i = 2; i > place; i--) {
            highscorePointsTab[i] = highscorePointsTab[i - 1];
            for (int j = 0; j < 3; j++) {
                highscoreNicksTab[place][j] = highscoreNicksTab[i - 1][j];
            }
        }
        highscorePointsTab[place] = points;
        for (int j = 0; j < 3; j++) {
            highscoreNicksTab[place][j] = nick[j];
        }
    }
}


int getRandomNumber(int y, int x) {
    srand(time(0));
    int num = (rand() % (x - y + 1)) + y;
    return num;
}


void createChars(char *pattern, int n) {
//    SEND_CMD(CG_RAM_ADDR);
//    for (int i = 0; i < 8; i++) {
//        SEND_CHAR(avatar[i]);
//    }
//    for (int i = 0; i < 8; i++) {
//        SEND_CHAR(point1[i]);
//    }
//    for (int i = 0; i < 8; i++) {
//        SEND_CHAR(point2[i]);
//    }
//    for (int i = 0; i < 8; i++) {
//        SEND_CHAR(point2[i]);
//    }
//    for (int i = 0; i < 8; i++) {
//        SEND_CHAR(point3[i]);
//    }
//    SEND_CMD(DD_RAM_ADDR);

    SEND_CHAR(n-1);
    SEND_CMD(0x40 + 8 * (n - 1));
    for (int i = 0; i < 8; i++) {
        SEND_CHAR(pattern[i]);
    }





}



//void initChars(void) {
////    createChars();
//
//
//}
