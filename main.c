#include <time.h>
#include "LCD.h"
#include "portyLcd.h"
#include <string.h>
// #include "notes.h" // co to ??
#include "msp430x14x.h" // TODO moze zmienic trzeba bedzie "" na <>

#include <time.h> // TODO chyba potrzebne ?
#include <stdlib.h> // TODO chyba potrzebne ?


//---------------- zmienne globalne -------------
unsigned int counter = 0; // zmienna do opóźniania działania przycisków
unsigned short pos = 0; // zmienna wyznaczająca szybkość zająca TODO zmienic
unsigned short option = 0; // zmienna wyboru opcji TODO zmienic
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
// initChars(); // inicjalizacja naszych znakow

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
    pos++; // zmienna inkrementowana zegarem A służaca do wyznaczania szybkości gry
    pos = pos % 100; // ograniczenie zmiennej
}


void menu(void) {
    // ------------------------- STEROWANIE: ---------------------
    // 1 przycisk: przewija liste menu w dol
    // 2 przycisk: przewija liste menu w gore
    // 3 przycisk: wybiera opcje
    // ------------------------------------------------------------
    switch (option) { // przesuwanie wskaźnika wyboru zależne od zmiennej option
        case 0:
            clearDisplay();
            SEND_CMD(DD_RAM_ADDR);
            writeText(" > GAME < ");
            SEND_CMD(DD_RAM_ADDR2);
            writeText("AUTHORS");
            break;
        case 1:
            clearDisplay();
            SEND_CMD(DD_RAM_ADDR);
            writeText("GAME");
            SEND_CMD(DD_RAM_ADDR2);
            writeText(" > AUTHORS < ");
            break;
        case 2:
            clearDisplay();
            SEND_CMD(DD_RAM_ADDR);
            writeText("AUTHORS");
            SEND_CMD(DD_RAM_ADDR2);
            writeText(" > HIGHSCORE < ");
            break;
    }
    if ((P4IN & BIT6) == 0) { // odczytanie stanu bitu P4.6 (jeśli przycisk jest wciśnięty)
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
        }
    } else if ((P4IN & BIT5) == 0) { // odczytanie stanu bitu P4.4 (jeśli przycisk jest wciśnięty)
        if (counter % 5 == 0) {
            counter = 0;
            if (option != 0) {
                option--; // dekrementacja zmiennej option - przesunięcie wskaźnika wyboru w górę
            } else {
                option = 2;
            }
        }
    } else if ((P4IN & BIT4) == 0) {
        if (counter % 5 == 0) {
            counter = 0;
            if (option != 3) {
                option++; // inkrementacja zmiennej option - przesunięcie wskaźnika wyboru w dół
            } else {
                option = 0;
            }
        }
    }
}

void game(void) {   //TODO opcjonalnie wyswietlanie gdzies zyc, moze z wlasnym znakiem serduszka
    // ------------------------- STEROWANIE: ---------------------
    // 1 przycisk: skierowanie postaci do dolu
    // 2 przycisk: skierowanie postaci do gory
    // 3 przycisk: chwytanie punktow
    // 4 przycisk: wyjscie do MENU
    // ------------------------------------------------------------
    P2OUT = P2OUT | BIT1; // ustawienie bitu P2.1 na stan wysoki (dioda status gaśnie)
    int playerPosition = 1; // pozycja gracza wrzerz od lewej
    int playerHeight = 1;   // pozycja gracza 1 - gorna linia; 2 - dolna linia
    int boostPosition = 12; // pozycja punktu do zebrania
    int rate = 60; // tempo
    int level = 1; // poziom
    int pointsBoost = 10; // punkty za boost (sa pozniej mnozone w zaleznosci od rodzaju boosta)
    int totalPoints = 0; // suma punktow
    int life = 3; // zycia - traci jak nie zlapie punktu
    int caughtPoints = 0;   // zlapane punkty na danym poziomie trudnosci
    int buttonPressed = -1; // wcisniecie przycisku 1 - nie wcisniety ; 0 - wcisniety; -1 - na starcie
    clearDisplay();     // czysci wyswietlacz
    while (1) {
        // ------ wyswietlanie znakow   TODO nie usuwac tego na razie bo przydatna sciagawka
//        SEND_CMD(DD_RAM_ADDR + 1); //0x80
//        createChars(avatar, 1);
//        SEND_CMD(DD_RAM_ADDR + 2); //0x80
//        createChars(point1, 2);
//        SEND_CMD(DD_RAM_ADDR + 3); //0x80
//        createChars(point2, 3);
//        SEND_CMD(DD_RAM_ADDR + 4); //0x80
//        createChars(point3, 4);
        //-------------
        for (long i = 0; i < 3000000; i++); // odczekanie

        SEND_CMD(DD_RAM_ADDR + playerPosition); // wybranie dobrej pozycji dla avatara
        createChars(avatar, 1); //wyswietlenie avatara

        boostPosition = 12; // pozycja wyjsciowa boosta - skrajna z prawej



        int line = getRandomNumber(1, 3);    // losuje liczbe od 1 do 2
        if (line == 1) {
            SEND_CMD(DD_RAM_ADDR + boostPosition); // wybieranie pozycji do wyswietlenia - linia 1
        } else {
            SEND_CMD(DD_RAM_ADDR2 + boostPosition); // wybieranie pozycji do wyswietlenia - linia 2
        }

        int typeOfPoint = getRandomNumber(1, 11);    // losuje liczbe od 1 do 11
        if (typeOfPoint >= 1 && typeOfPoint <= 5) {
            typeOfPoint = 1;    // 50 % szansy, mnoznik *1
        } else if (typeOfPoint >= 6 && typeOfPoint <= 8) {
            typeOfPoint = 2;    // 30% szansy, mnoznik *2
        } else {
            typeOfPoint = 3;    // 20% szansy, mnoznik *3
        }



        if (typeOfPoint == 1) {
            createChars(point1, 2); // wyswietlanie sie boosta 1 na pozycji startowej
        } else if (typeOfPoint == 2) {
            createChars(point2, 3); // wyswietlenie sie boosta 2 na pozycji startowej
        } else {
            createChars(point3, 4); // wyswietlanie sie boosta 3 na pozycji startowej
        }

        for (int i = boostPosition; i > 0; --i) {   // przesuwa boosty w lewo
            for (long j = 0; j < 3000000; j++); // odczekanie TODO uzaleznic oczekiwanie (szybkosc przesuwania sie) od zmiennej rate (timer A)

            if ((P4IN & BIT7) == 0) { // wyjscie - zakonczenie gry (trzeba bedzie przytrzymac przez chwile by wyszlo)
                return;
            }

            if ((P4IN & BIT6) == 0) {   // jesli klikne przycisk
                buttonPressed = 0;      // odnotowuje to i zlapie przy puszczeniu klawisza
            }

           if (line == 1) {    // zamazanie w odpowiedniej linii i miejscu starego znaku boosta
                SEND_CMD(DD_RAM_ADDR + boostPosition + 1); // wybieranie pozycji do wyswietlenia - linia 1
            } else {
                SEND_CMD(DD_RAM_ADDR2 + boostPosition + 1); // wybieranie pozycji do wyswietlenia - linia 2
            }
            SEND_CHAR(' '); // zamazanie poprzedniej pozycji boosta TODO check


            if (line == 1) {    // wyswietlanie w odpowiedniej linii przesuwajacego sie boosta
                SEND_CMD(DD_RAM_ADDR + boostPosition); // wybieranie pozycji do wyswietlenia - linia 1
            } else {
                SEND_CMD(DD_RAM_ADDR2 + boostPosition); // wybieranie pozycji do wyswietlenia - linia 2
            }

            if (typeOfPoint == 1) {     // po wybraniu odpowiedniej linii i miejsca wyswietlamy tam odpowiedni boost
                createChars(point1, 2); // przesuwanie sie boosta 1
            } else if (typeOfPoint == 2) {
                createChars(point2, 3); // przesuwanie sie boosta 2
            } else {
                createChars(point3, 4); // przesuwanie sie boosta 3
            }


            if ((P4IN & BIT4) == 0 && (playerHeight == 1)) {   // jesli klikne przycisk i gracz byl w gornej linii
                SEND_CMD(DD_RAM_ADDR + playerPosition); // chwilowy wybor pierwszej linii
                SEND_CHAR(' '); // zamazanie gornego avatara

                playerHeight = 2;   // avatar do drugiej linii
                SEND_CMD(DD_RAM_ADDR2 + playerPosition); // wybranie dobrej pozycji dla avatara
                createChars(avatar, 1); //wyswietlenie avatara na dolnej linii
            }

            if ((P4IN & BIT5) == 0 && (playerHeight == 2)) {   // jesli klikne przycisk i gracz byl w dolnej linii
                SEND_CMD(DD_RAM_ADDR2 + playerPosition);    // chwilowy wybor drugiej linii
                SEND_CHAR(' '); // zamazanie dolnego avatara

                playerHeight = 1;   // avatar do pierwszej linii
                SEND_CMD(DD_RAM_ADDR + playerPosition); // wybranie dobrej pozycji dla avatara
                createChars(avatar, 1); //wyswietlenie avatara na gornej linii
            }





            if ((P4IN & BIT6) == 1) {   // jesli puszcze przycisk
                buttonPressed = 1;      // odnotowuje to i sprawdzam czy zlapalem punkt
                break;
            }

            if (boostPosition <= playerPosition) {  // boost przelecial bohatera, za pozno klikniety przycisk
                break;
            }

        }

        P1OUT ^= BIT5; // ustawienie bitu P2.1 na stan niski (dioda status świeci)
        clearDisplay();

        if (boostPosition == playerPosition + 1) { // jesli punkt jest kratke przed postacia (jesli zlapany)
            totalPoints += (pointsBoost * typeOfPoint); // inkrementacja punktow o bazowe punkty * jego rodzaj (wage)
            caughtPoints++; // inkrementacja zdobytych na tym poziomie punktow
            if (caughtPoints == 3) {    // jesli zbierze 3 boosty
                caughtPoints = 0;   // przy wejsciu na nowy level zebrane boosty sie zeruja
                pointsBoost += 2; // przy wejsciu na nowy poziom inkrementacja wspolczynnika punktow o 2
                level++;    // inkrementacja levelu
                levelScreen(level); // wyswietlenie nowego levelu
                if (rate != 1) { // zmniejszenie rate wplywa na zwiekszenie tempa gry
                    rate -= 3;
                }
            }
        } else {    // nie udalo sie zlapac boosta
            life--;     // dekrementacja ilosci zyc
            if (life > 0) {     // jesli gracz ma wciaz zycia
                levelScreen(level);     // wyswietlenie jeszcze raz numeru tego samego levelu
            } else {    // jesli gracz nie ma juz zyc - koniec gry
                SEND_CMD(DD_RAM_ADDR);  // wysylanie danych na gorna linie wyswietlacza
                writeText("GAME OVER"); // wyswietlenie GAME OVER
                SEND_CMD(DD_RAM_ADDR2); // wysylanie danych na dolna linie wyswietlacza
                writeText("PTS: "); // wyswietlenie PTS: (Points:)
                writeNumber(totalPoints);   // wyswietlenie zdobytej liczby puntków
                for (long i = 0; i < 2000000; i++); // przerwa na ogladanie wyniku
                clearDisplay(); // czysci wyswietlacz
                return; // powrot do menu
            }
        }
        buttonPressed = -1; // przywrocenie zmiennej od przycisku  do pozycji startowej
        boostPosition = 12; // przywracam pozycje boosta do pozycji startowej




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
    writeText(" ");
    for (int i = 0; i < 3; i++) // wyświetlenie nicku
        SEND_CHAR(highscoreNicksTab[0][i]);

    SEND_CMD(DD_RAM_ADDR2);
    writeText("2. ");
    writeNumber(highscorePointsTab[1]);
    writeText(" ");
    for (int i = 0; i < 3; i++)
        SEND_CHAR(highscoreNicksTab[1][i]);

    for (long i = 0; i < 3000000; i++);
    clearDisplay();

    SEND_CMD(DD_RAM_ADDR);
    writeText("2. ");
    writeNumber(highscorePointsTab[1]);
    writeText(" ");
    for (int i = 0; i < 3; i++)
        SEND_CHAR(highscoreNicksTab[1][i]);

    SEND_CMD(DD_RAM_ADDR2);
    writeText("3. ");
    writeNumber(highscorePointsTab[2]);
    writeText(" ");
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
    clearDisplay();
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
    SEND_CHAR(n - 1);
    SEND_CMD(0x40 + 8 * (n - 1));
    for (int i = 0; i < 8; i++) {
        SEND_CHAR(pattern[i]);
    }


}



//void initChars(void) {
//// createChars();
//
//
//}