#include <time.h>
#include "lcd.h"
#include "portyLcd.h"
#include "notes.h"
#include <msp430x14x.h>

//---------------- zmienne globalne -------------
unsigned int counter = 0;
// zmienna do opóźniania działania przycisków
uint8 pos = 0;
// zmienna wyznaczająca szybkość zająca
uint8 option = 0;
// zmienna wyboru opcji
int highscorePointsTab[3] = {0, 0, 0};
// 1, 2, 3 miejsce
char highscoreNicksTab[3][3] = {{' ', ' ', ' '},
                                {' ', ' ', ' '},
                                {' ', ' ', ' '}}; // 1, 2, 3 miejsce
// własne znaki zapisane heksadecymalnie
char ly_bytes[] = {0x10, 0x10, 0x14, 0x18, 0x10, 0x10, 0x1F, 0x00};
// 0
char ol_bytes[] = {0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F, 0x04, 0x02};
// 1
char el_bytes[] = {0x0E, 0x11, 0x1F, 0x10, 0x11, 0x0E, 0x04, 0x02};
// 2
char rabbit_left_bytes[] = {0x00, 0x03, 0x06, 0x0C, 0x0C, 0x06, 0x06, 0x0A}; // 3
char rabbit_right_bytes[] = {0x00, 0x18, 0x0C, 0x06, 0x06, 0x0C, 0x0C, 0x0A}; // 4
char wolf_bytes[] = {0x11, 0x1B, 0x1F, 0x15, 0x1F, 0x0E, 0x04, 0x00};
// 5
int ly_char, ol_char, el_char, rabbit_left_char, rabbit_right_char, wolf_char;

void menu(void);

// menu główne gry
void game(void);

// gra
void desc(void);

// opis gry
void authors(void);

// autorzy
void highscore(void);

// najlepsze wyniki
void levelScreen(int);

// ekran poziomu
void saveHighscore(int); // zapis nicku i punktacji gracza
void writeText(unsigned char *); // wypisanie na ekran ciągu znaków
void writeNumber(int);

// wypisanie na ekran liczby
int getRandomNumber(int, int);

// losowanie liczby w zakresie (y, x)
void createChars(void);

// zapisanie nowych znaków do pamięci
void initChars(void);

// inicjacja nowych znaków i przypisanie wartości
// ================ main ================
void main(void) {
    P2DIR |= BIT1; // ustawienie bitu P2.1 (dioda status) na 1 (tryb wyjściowy)
    P4DIR |= 0x0C; // ustawienie bitu P4.2, P4.3 (buzzer) na 1 (tryb wyjściowy)
// P4DIR |= BIT2 | BIT3
    P4DIR &= 0x0F; // ustawienie bitu P4.4, P4.5, P4.6, P4.7 (buttony) na 0 (tryb wejściowy)
// P4DIR &= ~(BIT4 & BIT5 & BIT6 & BIT7)
    WDTCTL = WDTPW + WDTHOLD; // wyłączenie mechanizmu Watchdog
    InitPortsLcd(); // inicjalizacja portów LCD
    InitLCD();
    // inicjalizacja LCD
    clearDisplay(); // czyszczenie wyświetlacza
    initChars(); // inicjalizacja własnych znaków
// ustawienie Basic Clock Module na ACLK (zegar 8MHz) i podział częstotliwości przez 1 (8MHz)
    BCSCTL1 |= XTS; // ACLK = LFXT1 = HF XTAL 8MHz
    do {
        IFG1 &= ~OFIFG; // czyszczenie flgi OSCFault
        for (unsigned int i = 0xFF; i > 0; i--); // odczekanie
    } while ((IFG1 & OFIFG) == OFIFG); // dopóki OSCFault jest ciągle ustawiona
    BCSCTL1 |= DIVA_0;
    // ACLK = 8MHz / 1 = 8MHz
    BCSCTL2 |= SELM0 | SELM1; // MCLK = LFTX1 = ACLK
// ustawienie Timer_A na ...kHz, a przerwanie co ...ms
    TACTL = TASSEL_1 + MC_1 + ID_0; // wybór ACLK, ACLK / ... = ...kHz, tryb Up
    CCTL0 = CCIE;
    // włączenie przerwań od CCR0
    CCR0 = 40000;
    // podzielnik ...: przerwanie co ...ms
    _EINT(); // włączenie przerwań
    for (;;) // nieskończona pętla
    {
        _BIS_SR(LPM3_bits); // przejście do trybu LPM3
        menu();
        // przejście do menu
        counter++;
        // inkrementacja countera
        counter = counter % 30; // reset countera co 30
    }
}
// procedura obsługi przerwania od Timer A
#pragma vector = TIMERA0_VECTOR

__interrupt void Timer_A(void) {
    _BIC_SR_IRQ(LPM3_bits); // wyjście z trybu LPM3
    pos++;
    // zmienna inkrementowana zegarem A służaca do wyznaczania szybkości zająca
    pos = pos % 100; // ograniczenie zmiennej
}

// ================ main functions ================
void menu(void) {
    switch (option) { // przesuwanie wskaźnika wyboru zależne od zmiennej option
        case 0:
            SEND_CMD(DD_RAM_ADDR);
            writeText(" > GAME < ");
            SEND_CMD(DD_RAM_ADDR2);
            writeText("DESC");
            break;
        case 1:
            SEND_CMD(DD_RAM_ADDR);
            writeText("GAME");
            SEND_CMD(DD_RAM_ADDR2);
            writeText(" > DESC < ");
            break;
        case 2:
            SEND_CMD(DD_RAM_ADDR);
            writeText("DESC");
            SEND_CMD(DD_RAM_ADDR2);
            writeText(" > AUTHORS < ");
            break;
        case 3:
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
            case 1: // opis gry
                desc();
                option = 1;
                break;
            case 2: // autorzy
                authors();
                option = 2;
                break;
            case 3: // najlepsi zawodnicy
                highscore();
                option = 3;
                break;
        }
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
    P2OUT = P2OUT | BIT1; // ustawienie bitu P2.1 na stan wysoki (dioda status gaśnie)
// ustawienie gry
    int wPos = getRandomNumber(3, 12); // pozycja wilka (losowana w zakresie (3,12))
    int rPos = 0;
    // pozycja zająca
    int rate = 60;
    // tempo
    int direction = 1;
    // kierunek
    unsigned int level = 1;
    // poziom
    int points = 1000;
    // max punkty na poziom
    int totalPoints = 0;
    // suma punktów
    int btnPressed = 0;
    while {
        (1)
        if ((P4IN & BIT6) == 0 && btnPressed == 0) // odczytanie stanu bitu P4.6 (jeśli przycisk jest
            wciśnięty - sprawdzenie, czy
        gracz
        trafił)
        {
            P1OUT = P1OUT | BIT5; // ustawienie bitu P2.1 na stan niski (dioda status świeci)
            btnPressed = 1;
            clearDisplay();
// jeśli gracz złapał zająca (zając był w kratce przed wilkiem)
            if ((direction == 1 && rPos - 1 == wPos) || (direction == -1 && rPos + 1 == wPos)) {
                level++;
                // level up
                levelScreen(level); // wyświetlenie ekranu poziomu
                if (rate != 1) // zmniejszenie zmiennej rate -> zwiększenie tempa gry
                    rate -= 3;
                wPos = getRandomNumber(3, 12); // pozycja wilka (losowana w zakresie (3,12))
                rPos = 0;
                // pozycja zająca
                direction = 1;
                totalPoints += points; // dodanie zdobytych punktów z poziomu do ogólnej punktacji
                points = 1000;
                btnPressed = 0;
            } else // jeśli gracz nie złapał zająca
            {
                writeText("GAME OVER"); // wyświetlenie GAME OVER oraz zdobytej liczby puntków
                SEND_CMD(DD_RAM_ADDR2);
                writeNumber(totalPoints);
                for (long i = 0; i < 2000000; i++); // przerwa
                clearDisplay();
                saveHighscore(totalPoints); // przejście do ekranu zapisu punktów
// saveHighscore(totalPoints); // ekran zapisu nicku i punktacji
// if (totalPoints > highscorePoints) // jeśli wynik zalicza się do highscore
//
                highscorePoints = totalPoints; // zapis punktacji
                for (long i = 0; i < 2000000; i++);
                break; // wyjście z pętli while -> powrót do menu
            }
        }
        if (btnPressed == 0) // jeśli przycisk (P4.6) nie został wciśnięty
        {
            SEND_CMD(DD_RAM_ADDR2 + wPos); // ustawienie wilka na pozycji
            SEND_CHAR(wolf_char);
            // wysłanie znaku wilka
        }
        if (pos % rate == 0 && btnPressed == 0) // jeśli minęła odpowiednia ilość czasu na podstawie rate
        {
            pos = 0;
            SEND_CMD(CUR_HOME);
            SEND_CMD(DD_RAM_ADDR + rPos); // ustawienie zająca na pozycji
            if (direction == 1) // jeśli zając skacze w prawo
            {
                SEND_CHAR(rabbit_right_char); // wysłanie znaku zająca w prawo
                if (rPos != 0) {
                    SEND_CMD(DD_RAM_ADDR + rPos - 1); // zamazanie poprzedniej pozycji zająca
                    SEND_CHAR(' ');
                }
            } else if (direction == -1) // jeśli zając skacze w lewo
            {
                SEND_CHAR(rabbit_left_char); // wysłanie znaku zająca w lewo
                if (rPos != 15) {
                    SEND_CMD(DD_RAM_ADDR + rPos + 1); // zamazanie poprzedniej pozycji zająca
                    SEND_CHAR(' ');
                }
            }
            if ((rPos == 15 && direction == 1) || (rPos == 0 && direction == -1)) // jeśli zając doszedł do
                granicy direction = direction * (-1);
            // zmiana kierunku
            if (direction == 1) // inkrementacja lub dekrementacja pozycji zająca w zależności od kierunku
                poruszania
                rPos++;
            else
                rPos--;
            if (points != 100)
// jeśli zając bezpiecznie ominął wilka
                if ((direction == 1 && rPos - 2 == wPos) || (direction == -1 && rPos + 2 == wPos))
                    points -= 100;
            // -100 punktów od max punktacji za poziom, ale nie gdy zostało już tylko 100
        }
    }
}

void desc(void) {
// przewijany w dół opis gry
    writeText("Zlap zajaca jak");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("najszybciej!");
    for (long i = 0; i < 4000000; i++);
    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("najszybciej!");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("Uwaga! Z czasem");
    for (long i = 0; i < 4000000; i++);
    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("bedzie szybszy");
    for (long i = 0; i < 4000000; i++);
}

void authors() {
// przewijani w dół autorzy gry
    SEND_CMD(DD_RAM_ADDR);
    writeText("Kamila");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("Adamska");
    for (long i = 0; i < 3000000; i++);
    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("Patrycja");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("Birylo");
    for (long i = 0; i < 3000000; i++);
    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("Dominik");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("Bukrejewski");
    for (long i = 0; i < 3000000; i++);
    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("Marek");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("Chojnowski");
    for (long i = 0; i < 3000000; i++);
}

void highscore(void) {
// przewijana w dół lista 3 najlepszych graczy wraz z punktacją
    SEND_CMD(DD_RAM_ADDR);
    writeText("1. ");
    writeNumber(highscorePointsTab[0]); // wyświetlenie punktacji
    writeText("
              ");
    for (int i = 0; i < 3; i++) // wyświetlenie nicku
        SEND_CHAR(highscoreNicksTab[0][i]);
    SEND_CMD(DD_RAM_ADDR2);
    writeText("2. ");
    writeNumber(highscorePointsTab[1]);
    writeText("
              ");
    for (int i = 0; i < 3; i++)
        SEND_CHAR(highscoreNicksTab[1][i]);
    for (long i = 0; i < 3000000; i++);
    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("2. ");
    writeNumber(highscorePointsTab[1]);
    writeText("
              ");
    for (int i = 0; i < 3; i++)
        SEND_CHAR(highscoreNicksTab[1][i]);
    SEND_CMD(DD_RAM_ADDR2);
    writeText("3. ");
    writeNumber(highscorePointsTab[2]);
    writeText("
              ");
    for (int i = 0; i < 3; i++)
        SEND_CHAR(highscoreNicksTab[2][i]);
    for (long i = 0; i < 3000000; i++);
}

void levelScreen(int level) {
    writeText("LEVEL "); // wypisanie numeru poziomu
    writeNumber(level);
    for (long i = 0; i < 10000; i++);
}

void saveHighscore(int points) {
    int letter = 0;
    // wybierana litera
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
        if ((P4IN & BIT7) == 0 && nick[curr] != '_')
// przycisk P4.7 -> zapis obecnej litery i przejście do kolejnej
        {
            letter = 0;
            curr++;
            if (curr == 3) // jeśli została zapisana ostatnia litera -> wyjdź z pętli
            {
                SEND_CMD(DD_RAM_ADDR2 + 6);
                writeText("SAVED");
                for (long i = 0; i < 100000; i++);
                SEND_CMD(DD_RAM_ADDR2 + 6);
                SEND_CHAR(nick[0]);
                SEND_CHAR(' ');
                SEND_CHAR(nick[1]);
                SEND_CHAR(' ');
                SEND_CHAR(nick[2]);
                break;
            }
        } else if ((P4IN & BIT4) == 0) // odczytanie stanu bitu P4.4 (jeśli przycisk jest wciśnięty)
        {
            if (counter % 10 == 0) {
                counter = 0;
                if (letter != 0)
                    letter--; // dekrementacja zmiennej letter - zmiana litery
                nick[curr] = letter + 65; // zmiana liczby na literę w ASCII
            }
        } else if ((P4IN & BIT5) == 0) // odczytanie stanu bitu P4.% (jeśli przycisk jest wciśnięty)
        {
            if (counter % 10 == 0) {
                counter = 0;
                if (letter < 25)
                    letter++; // inkrementacja zmiennej letter - zmiana litery
                nick[curr] = letter + 65; // zmiana liczby na literę w ASCII
            }
        }
    }
    for (long i = 0; i < 10000; i++);
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
            for (int j = 0; j < 3; j++)
                highscoreNicksTab[place][j] = highscoreNicksTab[i - 1][j];
        }
        highscorePointsTab[place] = points;
        for (int j = 0; j < 3; j++)
            highscoreNicksTab[place][j] = nick[j];
    }
}

// ================ side functions ================
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

int getRandomNumber(int y, int x) {
    srand(time(0));
    int num = (rand() % (x - y + 1)) + y;
    return num;
}

void createChars() {
    SEND_CMD(CG_RAM_ADDR);
    for (int i = 0; i < 8; i++)
        SEND_CHAR(ly_bytes[i]);
    for (int i = 0; i < 8; i++)
        SEND_CHAR(ol_bytes[i]);
    for (int i = 0; i < 8; i++)
        SEND_CHAR(el_bytes[i]);
    for (int i = 0; i < 8; i++)
        SEND_CHAR(rabbit_left_bytes[i]);
    for (int i = 0; i < 8; i++)
        SEND_CHAR(rabbit_right_bytes[i]);
    for (int i = 0; i < 8; i++)
        SEND_CHAR(wolf_bytes[i]);
}

SEND_CMD(DD_RAM_ADDR);

void initChars() {
    createChars();
    ly_char = 0;
    ol_char = 1;
    el_char = 2;
    rabbit_left_char = 3;
    rabbit_right_char = 4;
    wolf_char = 5;
}
