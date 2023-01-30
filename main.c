#include <time.h>
#include "LCD.h"
#include "portyLcd.h"
#include <string.h>
#include "msp430x14x.h"
#include <stdlib.h>


//---------------- zmienne globalne -------------
unsigned int counter = 0; // zmienna do opozniania w highScore
unsigned short option = 0; // zmienna wyboru opcji w menu
int highScorePointsTab[3] = {0, 0, 0}; // 1, 2, 3 miejsce
char highScoreNicksTab[3][3] = {{' ', ' ', ' '},
                                {' ', ' ', ' '},
                                {' ', ' ', ' '}}; // 1, 2, 3 miejsce

//--------------------------------
// wlasne znaki
char avatar[8] = {0x03, 0x07, 0x0E, 0x1C, 0x1C, 0x0E, 0x07, 0x03}; // lapki
char point1[8] = {0x00, 0x00, 0x00, 0x06, 0x06, 0x00, 0x00, 0x00}; // kwadracik
char point2[8] = {0x00, 0x00, 0x06, 0x0F, 0x0F, 0x06, 0x00, 0x00}; // koleczko
char point3[8] = {0x00, 0x00, 0x00, 0x04, 0x0E, 0x1F, 0x00, 0x00}; // trojkacik
char heart[8] = {0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00}; // serce
//-------------------------------------------

// Naglowki funkcji

void menu(void); // glowne menu gry

void game(void); // kod gry

void authors(void); // autorzy

void highScore(void); // tablica najlepszych wynikow

void saveHighScore(int); // zapisanie nicku i wyniku gracza

void levelScreen(int); // ekran poziomu

void writeText(unsigned char *); // wypisanie na ekran ciagu znakow

void writeNumber(int); // wypisanie na ekran liczby

void gameOver(int *totalPoints);

void endGame(int *totalPoints);

void createChars(char *, int); // zapisanie wygenerowanych znakow w pamieci

short avatarMove(int *playerHeight, int *playerPosition); // przesuwanie sie postaci
//-------------------------------------------


void main(void) {
    P2DIR |= BIT1; // ustawienie bitu P2.1 (dioda status) na 1 (tryb wyjscciowy)
    P4DIR |= 0x0C; // ustawienie bitu P4.2, P4.3 (buzzer) na 1 (tryb wyjscciowy)
    P4DIR &= 0x0F; // ustawienie bitu P4.4, P4.5, P4.6, P4.7 (buttony) na 0 (tryb wejscciowy)
    WDTCTL = WDTPW + WDTHOLD; // wylaczenie mechanizmu Watchdog
    InitPortsLcd(); // inicjalizacja portow LCD
    InitLCD(); // inicjalizacja LCD
    clearDisplay(); // czyszczenie wyswietlacza
    Delayx100us(20); // odczekanie

    BCSCTL1 |= XTS; // ACLK = LFXT1 = HF XTAL 8MHz
    do {
        IFG1 &= ~OFIFG; // Czyszczenie flagi OSCFault
        for (unsigned char i = 0xFF; i > 0; i--); // odczekanie
    } while ((IFG1 & OFIFG) == OFIFG); // dopóki OSCFault jest ci1gle ustawiona

    BCSCTL1 |= DIVA_0; // ACLK=8 MHz
    BCSCTL2 |= SELM0 | SELM1; // MCLK= LFTX1 =ACLK
    Delayx100us(20); // odczekanie

/* ustawienie Timer_A na ...kHz, a przerwanie co ...ms */
    TACTL = TASSEL_1 + MC_1 + ID_0; // wybor ACLK, ACLK / ... = ...kHz, tryb Up
    CCTL0 = CCIE; // wylaczenie przerwan od CCR0
    CCR0 = 40000; // podzielnik ...: przerwanie co ...ms

    for (;;) {
        menu(); // przejscie do menu
    }
}


void menu(void) {
// ------------------------- STEROWANIE: ---------------------
// 1 przycisk: przewija liste menu w dol
// 2 przycisk: przewija liste menu w gore
// 3 przycisk: wybiera opcje
// ------------------------------------------------------------
    switch (option) { // przesuwanie wskaznika wyboru zaleznie od zmiennej option
        case 0:
            clearDisplay(); // czyszczenie wyswietlacza
            SEND_CMD(DD_RAM_ADDR); // wybranie 1 linii
            writeText(" > GAME < "); // wyswietlenie na wyswietlaczu
            SEND_CMD(DD_RAM_ADDR2); // wybranie 2 linii
            writeText("AUTHORS"); // wyswietlenie na wyswietlaczu
            break;
        case 1:
            clearDisplay(); // czyszczenie wyswietlacza
            SEND_CMD(DD_RAM_ADDR); // wybranie 1 linii
            writeText("GAME"); // wyswietlenie na wyswietlaczu
            SEND_CMD(DD_RAM_ADDR2); // wybranie 2 linii
            writeText(" > AUTHORS < "); // wyswietlenie na wyswietlaczu
            break;
        case 2:
            clearDisplay(); // czyszczenie wyswietlacza
            SEND_CMD(DD_RAM_ADDR); // wybranie 1 linii
            writeText("AUTHORS"); // wyswietlenie na wyswietlaczu
            SEND_CMD(DD_RAM_ADDR2); // wybranie 2 linii
            writeText(" > HIGHSCORE < "); // wyswietlenie na wyswietlaczu
            break;
    }

    for (int i = 16; i > 0; --i) { // odczekanie
        Delayx100us(255);
    }

    while (((P4IN & BIT6) == BIT6) && ((P4IN & BIT5) == BIT5) &&
           ((P4IN & BIT4) == BIT4)) { // detekcja wcisniecia dowolnego przycisku
    };

    if ((P4IN & BIT6) == 0) { // jesli trzeci przycisk zostal puszczony i uprzednio klinkety
        clearDisplay();
        switch (option) { // wlaczenie wybranej opcji
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

    } else if ((P4IN & BIT4) == 0) { // jesli pierwszy przycisk zostal klikniety
        option++;
        option = option % 3;

    } else if ((P4IN & BIT5) == 0) { // jesli drugi przycisk zostal klikniety
        if (option == 0) {
            option = 2;
        } else {
            option--;
        }
    }
}


void game(void) {
// ------------------------- STEROWANIE: ---------------------
// 1 przycisk: skierowanie postaci do dolu
// 2 przycisk: skierowanie postaci do gory
// 3 przycisk: chwytanie punktow
// 4 przycisk: wyjscie do MENU
// ------------------------------------------------------------
    P2OUT = P2OUT | BIT1; // ustawienie bitu P2.1 na stan wysoki - dioda status gasnie
    int playerPosition = 1; // pozycja gracza wrzerz od lewej
    int playerHeight = 1; // pozycja gracza 1 - gorna linia; 2 - dolna linia
    int boostPosition = 12; // pozycja punktu do zebrania
    int level = 1; // poziom
    int tar = 0; // inicjacja zmiennej ktora posluzy do odczytu wartosci z timeraA
    int pointsBoost = 10; // punkty za boost (sa pozniej mnozone w zaleznosci od rodzaju boosta)
    int totalPoints = 0; // suma punktow
    int life = 3; // zycia - traci jak nie zlapie punktu
    int caughtPoints = 0; // zlapane punkty na danym poziomie trudnosci
    char lifeChar = '3'; // znak zyc, domyslnie jako zyc, sluzy do wyswietlenia na wyswietlaczu

    clearDisplay(); // czysci wyswietlacz
    while (1) {
        tar = TAR; // odczyt wartosci z licznika
        playerHeight = 1; // linia w ktorej pojawia sie gracz na poczatku rundy
        if (level > 6) { // czy ukonczyles gre - jest 90 poziomow
            endGame(&totalPoints); // ekran konca gry
            return; // powrot do menu
        }
        SEND_CMD(DD_RAM_ADDR + playerPosition); // wybranie dobrej pozycji dla avatara
        createChars(avatar, 1); //wyswietlenie avatara

        boostPosition = 12; // pozycja wyjsciowa boosta - skrajna z prawej


// losowanie linii na ktorej pojawi sie boost
        int line = tar % 2 + 1; // losuje liczbe od 1 do 2 na podstawie wartosci z Timera A
        if (line == 1) {
            SEND_CMD(DD_RAM_ADDR + boostPosition); // wybieranie pozycji do wyswietlenia - linia 1
        } else {
            SEND_CMD(DD_RAM_ADDR2 + boostPosition); // wybieranie pozycji do wyswietlenia - linia 2
        }

// losowanie jaki to bedzie boost
        int typeOfPoint = (tar % 10) + 1; // losuje liczbe od 1 do 10 na podstawie wartosci z Timera A
        if (typeOfPoint >= 1 && typeOfPoint <= 5) {
            typeOfPoint = 1; // 50 % szansy, mnoznik *1
        } else if (typeOfPoint >= 6 && typeOfPoint <= 8) {
            typeOfPoint = 2; // 30% szansy, mnoznik *2
        } else {
            typeOfPoint = 3; // 20% szansy, mnoznik *3
        }

// wyswietlenie odpowiedniego boosta na pozycji startowej
        if (typeOfPoint == 1) {
            createChars(point1, 2); // wyswietlanie sie boosta 1 na pozycji startowej
        } else if (typeOfPoint == 2) {
            createChars(point2, 3); // wyswietlenie sie boosta 2 na pozycji startowej
        } else {
            createChars(point3, 4); // wyswietlanie sie boosta 3 na pozycji startowej
        }

// dzialanie i wizualizacja polegajace na przemieszczaniu sie boosta w strone postaci
        for (boostPosition = 12; boostPosition > 0; --boostPosition) {
            for (long j = 0; j < (50000 - (level * 5000)); j++) { // szybkosc gry, z kazdym levelem bedzie szybsza
                if (!(((P4IN & BIT5) == BIT5) && ((P4IN & BIT4) == BIT4))) { // sprawdzenie czy gracz nie wykonal ruchu
                    avatarMove(&playerHeight, &playerPosition); // ruch bohatera (w gore lub w dol)
                }
            }

// wyswietlenie znaku serca i liczby zyc jakie ma bohater w prawym gornym rogu wyswietlacza
            lifeChar = life + 48; // konwersja liczby zyc na chara do wyswietlenia jej na wyswietlacz
            SEND_CMD(DD_RAM_ADDR + 14); // wybranie 14 pozycji na pierwszej linii
            createChars(heart, 5); // wyslanie serca
            SEND_CMD(DD_RAM_ADDR + 15); // wybranie 15 pozycji na pierwszej linii
            SEND_CHAR(lifeChar); // wyslanie liczby serc

            if ((P4IN & BIT7) == 0) { // wyjscie - zakonczenie gry (trzeba przytrzymac przez chwile)
                return; // powrot do menu
            }


            if (line == 1) { // zamazanie w odpowiedniej linii i miejscu starego znaku boosta
                SEND_CMD(DD_RAM_ADDR + boostPosition + 1); // wybieranie pozycji do wyswietlenia - linia 1
            } else {
                SEND_CMD(DD_RAM_ADDR2 + boostPosition + 1); // wybieranie pozycji do wyswietlenia - linia 2
            }
            SEND_CHAR(' '); // zamazanie poprzedniej pozycji boosta


            if (line == 1) { // wyswietlanie w odpowiedniej linii przesuwajacego sie boosta
                SEND_CMD(DD_RAM_ADDR + boostPosition); // wybieranie pozycji do wyswietlenia - linia 1
            } else {
                SEND_CMD(DD_RAM_ADDR2 + boostPosition); // wybieranie pozycji do wyswietlenia - linia 2
            }


            if (typeOfPoint == 1) { // po wybraniu odpowiedniej linii i miejsca wyswietlamy tam odpowiedni boost
                createChars(point1, 2); // przesuwanie sie boosta 1
            } else if (typeOfPoint == 2) {
                createChars(point2, 3); // przesuwanie sie boosta 2
            } else {
                createChars(point3, 4); // przesuwanie sie boosta 3
            }


            if (playerPosition == boostPosition - 1) { // jesli boost bedzie przed avatarem
                break; // przestaje przemieszczac boosta
            }

            if (boostPosition <= playerPosition) { // boost przelecial bohatera
                break; // przestaje przemieszczac boosta
            }

        }

        clearDisplay(); // czyszczenie wyswietlacza


        if (line == playerHeight) { // jesli punkt jest w tej samej linii co postac w momencie lapania
            totalPoints += (pointsBoost * typeOfPoint); // inkrementacja punktow o bazowe punkty * jego rodzaj (wage)
            caughtPoints++; // inkrementacja zdobytych na tym poziomie punktow
            if (caughtPoints == 3) { // jesli zbierze 3 boosty
                caughtPoints = 0; // przy wejsciu na nowy level zebrane boosty sie zeruja
                pointsBoost += 2; // przy wejsciu na nowy poziom inkrementacja wspolczynnika punktow o 2
                level++; // inkrementacja levelu
                levelScreen(level); // wyswietlenie nowego levelu
            }
        } else { // jesli nie udalo sie zlapac boosta
            life--; // dekrementacja ilosci zyc
            if (life > 0) { // jesli gracz ma wciaz zycia
                levelScreen(level); // wyswietlenie jeszcze raz numeru tego samego levelu
            } else { // jesli gracz nie ma juz zyc - koniec gry
                gameOver(&totalPoints); // ekan koncowy gry
                return; // powrot do menu
            }
        }
        boostPosition = 12; // przywrocenie pozycji boosta do pozycji startowej
    }
}

void endGame(int *totalPoints) {
    SEND_CMD(DD_RAM_ADDR); // wysylanie danych na gorna linie wyswietlacza
    writeText("CONGR."); // wyswietlenie CONGRATULATIONS na wyswietlacz
    SEND_CMD(DD_RAM_ADDR2); // wysylanie danych na dolna linie wyswietlacza
    writeText("PTS: "); // wyswietlenie PTS: (Points:)
    saveHighScore(*totalPoints); // przejscie do zapisywania wyniku
    for (long i = 0; i < 8000000; i++); // przerwa na ogladanie wyniku
    clearDisplay(); // czysci wyswietlacz
}

void gameOver(int *totalPoints) {
    SEND_CMD(DD_RAM_ADDR); // wysylanie danych na gorna linie wyswietlacza
    writeText("GAME OVER"); // wyswietlenie GAME OVER na wyswietlacz
    SEND_CMD(DD_RAM_ADDR2); // wysylanie danych na dolna linie wyswietlacza
    writeText("PTS: "); // wyswietlenie PTS: (Points:)
    saveHighScore(*totalPoints); // przejscie do zapisywania wyniku

    for (long i = 0; i < 5000000; i++); // przerwa na ogladanie wyniku
    clearDisplay();
}

void authors() {
// przewijani w dol‚ autorzy gry
    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("Mateusz");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("Horczak");
    for (long i = 0; i < 3000000; i++);

    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("Szymon");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("Lupinski");
    for (long i = 0; i < 3000000; i++);

    clearDisplay();
    SEND_CMD(DD_RAM_ADDR);
    writeText("Konrad");
    SEND_CMD(DD_RAM_ADDR2);
    writeText("Jankowski");
    for (long i = 0; i < 3000000; i++);
}


void highScore(void) {
// przewijana w dol‚ lista 3 najlepszych graczy wraz z punktacja
    SEND_CMD(DD_RAM_ADDR); // wybranie 1 linii wyswietlacza LCD
    writeText("1. "); // wyswietlenie 1.
    writeNumber(highScorePointsTab[0]); // wyswietlenie punktacji
    writeText(" "); // wyswietlenie spacji
    for (int i = 0; i < 3; i++) { // wyswietlenie nicku
        SEND_CHAR(highScoreNicksTab[0][i]); // wyslanie po jednym znakow nicku
    }

    SEND_CMD(DD_RAM_ADDR2); // wybranie 2 linii wyswietlacza LCD
    writeText("2. "); // wyswietlenie 2.
    writeNumber(highScorePointsTab[1]); // wyswietlenie punktacji
    writeText(" "); // wyswietlenie spacji
    for (int i = 0; i < 3; i++) { // wyswietlenie nicku
        SEND_CHAR(highScoreNicksTab[1][i]); // wyslanie po jednym znaku nicku
    }

    for (long i = 0; i < 3000000; i++); // odczekanie
    clearDisplay(); // czyszczenie wyswietlacza

    SEND_CMD(DD_RAM_ADDR); // wybranie 1 linii wyswietlacza LCD
    writeText("2. "); // wyswietlenie 2.
    writeNumber(highScorePointsTab[1]); // wyswietlenie punktacji
    writeText(" "); // wyswietlenie spacji
    for (int i = 0; i < 3; i++) { // wyswietlenie nicku
        SEND_CHAR(highScoreNicksTab[1][i]); // wyslanie po jednym znaku nicku
    }

    SEND_CMD(DD_RAM_ADDR2); // wybranie 2 linii wyswietlacza LCd
    writeText("3. "); // wyswietlenie 3.
    writeNumber(highScorePointsTab[2]); // wyswietlenie punktacji
    writeText(" "); // wyswietlenie spacji
    for (int i = 0; i < 3; i++) { //wyswietlenie nicku
        SEND_CHAR(highScoreNicksTab[2][i]); // wyslanie po jednym znaku nicku
    }

    for (long i = 0; i < 3000000; i++); // odczekanie
}


void writeText(unsigned char *text) {
    for (int i = 0; i < strlen(text); i++) { // wypisywanie po kolei znakow z tablicy
        SEND_CHAR(text[i]); // iterowanie po slowie i wypisywanie znaku
    }
}


void writeNumber(int x) {
    if (x >= 10) { // jesli liczba nie jest cyfra
        writeNumber(x / 10); // rekurencyjne wywoluje reszte z dzielenia
    }
    int number = x % 10 + 48; // zamiana liczby na znak ASCII
    SEND_CHAR(number); // wyslanie znaku po cyfrze
}


void levelScreen(int level) {
    writeText("LEVEL "); // wyswietlenie na wyswietlacz LEVEL
    writeNumber(level); // wyswietlenie numeru levelu
    for (long i = 2000000; i > 0; --i); // petla odczekujaca
    clearDisplay(); // czyszczenie wyswietlacza
}


void saveHighScore(int points) {
    int letter = 0; // pozycja obecnej litery
    char nick[3] = {'_', '_', '_'}; // 3 literowy nick gracza
    int current = 0; // inicjalizacja current

    counter = 0; // zerowanie licznika

    while (1) {
        counter++; // incrementacja licznika
        counter = counter % 30; // licznik modulo 30

        SEND_CMD(DD_RAM_ADDR2 + 5); // wybranie 5 pozycji na 2 linii
        writeNumber(points); // wyswietlenie zdobytych punktow
        SEND_CMD(DD_RAM_ADDR + 11); // wybranie 11 pozycji na 1 linii

// pola zapisu nicku
        SEND_CHAR(nick[0]); // wyslanie 1 litery nicku
        SEND_CHAR(' '); // wyslanie spacji
        SEND_CHAR(nick[1]); // wyslanie 2 litery nicku
        SEND_CHAR(' '); // wyslanie spacji
        SEND_CHAR(nick[2]); // wyslanie trzeciej litery nicku
        if ((P4IN & BIT7) == 0 && nick[current] != '_') { // przycisk 4 zapisuje obecna litery i przechodzi do kolejnej
            letter = 0; // wyzerowanie litery
            current++; // inkrementacja current

            if (current == 3) { // jesli zostala zapisana ostatnia literka nicku to wychodzi z petli
                SEND_CMD(DD_RAM_ADDR + 11); // wybranie 11 pozycji na 1 linii
                writeText("SAVED"); // wyswietlenie napisu SAVED

                for (long i = 0; i < 100000; i++); // odczekanie

                SEND_CMD(DD_RAM_ADDR + 11); // wybranie 11 pozycji na 1 linii
                SEND_CHAR(nick[0]); // wyswietlenie 1 litera nicku
                SEND_CHAR(' '); // wyswietlenie spacji
                SEND_CHAR(nick[1]); // wyswietlenie 2 litery nicku
                SEND_CHAR(' '); // wyswietlenie spacji
                SEND_CHAR(nick[2]); // wyswietlenie trzeciej litery nicku
                break; // wyjscie z petli
            }
        } else if ((P4IN & BIT4) == 0) { // odczytanie stanu bitu P4.4 (jeĹ›li przycisk jest wciĹ›niÄ™ty)
            if (counter % 10 == 0) {
                counter = 0; // zerowanie licznika
                if (letter != 0) {
                    letter--; // dekrementacja zmiennej letter - zmiana litery
                }
                nick[current] = letter + 65; // zmiana liczby na literke w ASCII
            }
        } else if ((P4IN & BIT5) == 0) { // odczytanie stanu bitu P4.5 (jesli przycisk jest wcisniety)
            if (counter % 10 == 0) {
                counter = 0; // zerowanie licznika
                if (letter < 25)
                    letter++; // inkrementacja zmiennej letter - zmiana litery
                nick[current] = letter + 65; // zmiana liczby na literke w ASCII
            }
        }
    }
    for (long i = 0; i < 10000; i++); // odczekanie

// zapis wyniku z punktacja i nickiem w odpowiednim miejscu na liscie
    int place = -1; // inicjalizacja zmiennej place do wybrania miejsca dla nowego wyniku
    for (int i = 0; i < 3; i++) { // petla iterujaca po top3
        if (points >= highScorePointsTab[i]) { // jesli wynik jest lepszy od ktoregos z dotychczasowych
            place = i; // to nowy wynik ma byc w tym miejscu
            break;
        }
    }
    if (place != -1) {
        for (int i = 2; i > place; i--) {
            highScorePointsTab[i] = highScorePointsTab[i - 1]; // aktualizacja tabeli wynikow
            for (int j = 0; j < 3; j++) {
                highScoreNicksTab[place][j] = highScoreNicksTab[i - 1][j]; // aktalizacja tabeli wynikow
            }
        }
        highScorePointsTab[place] = points; // aktualizacja tabeli wynikow
        for (int j = 0; j < 3; j++) {
            highScoreNicksTab[place][j] = nick[j]; // aktualizacja tabeli wynikow
        }
    }
}


void createChars(char *pattern, int n) {
    SEND_CHAR(n - 1); // wybiera n-1 pozycje z linii
    SEND_CMD(0x40 + 8 * (n - 1)); // wrzuca na LCD znak wlasny
    for (int i = 0; i < 8; i++) {
        SEND_CHAR(pattern[i]); // wyslanie znaku
    }
}


short avatarMove(int *playerHeight, int *playerPosition) {

// przesuniecie postaci w dol
    if (((P4IN & BIT4) == 0) && (*playerHeight == 1)) { // jesli klikne przycisk i gracz byl w gornej linii
        P2OUT ^= BIT1; // zapalenie sie diody
        SEND_CMD(DD_RAM_ADDR + *playerPosition); // chwilowy wybor pierwszej linii
        SEND_CHAR(' '); // zamazanie gornego avatara

        *playerHeight = 2; // avatar do drugiej linii
        SEND_CMD(DD_RAM_ADDR2 + *playerPosition); // wybranie dobrej pozycji dla avatara
        createChars(avatar, 1); //wyswietlenie avatara na dolnej linii
        return 1;
    }

// przesuniecie postaci w gore
    else if (((P4IN & BIT5) == 0) && (*playerHeight == 2)) { // jesli klikne przycisk i gracz byl w dolnej linii
        P2OUT ^= BIT1; // zapalenie sie diody
        SEND_CMD(DD_RAM_ADDR2 + *playerPosition); // chwilowy wybor drugiej linii
        SEND_CHAR(' '); // zamazanie dolnego avatara

        *playerHeight = 1; // avatar do pierwszej linii
        SEND_CMD(DD_RAM_ADDR + *playerPosition); // wybranie dobrej pozycji dla avatara
        createChars(avatar, 1); //wyswietlenie avatara na gornej linii
        Delayx100us(255); // odczekanie
        return 1;
    }
    return 1;
}
