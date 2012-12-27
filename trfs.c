/*
 * TaRyFikator by Maciej Korzeń <maciek@korzen.org>
 *
 * $Id: trfs.c,v 1.35 2003/02/02 18:32:03 eaquer Exp $
 * 
 * Copyright (C) 2002  Maciej Korzeń
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 675
 * Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

/*
 * Ze względy na "dziwną" implementację strftime i strptime w Linuksie,
 * trzeba się dostosować.
 */
#ifdef LINUX
#define TIME_CHAR	"%c"
#endif

#ifdef FREEBSD
#define	TIME_CHAR	"%+"
#endif

#define VERSION	"0.4"

/* Jaśniejszy tekst do drukowania obramowań. */
#define BOLD "\033[1;37m"
#define NORMAL "\033[0m"

/*
 * Do określenia z którego pliku mamy odczytywać informacje o połączeniach.
 * (Patrz funkcja pobierz()).
 */
u_int             plik;

/*
 * Wyświetla podany tekst rozjaśnioną czcionką. Używane do rysowania ramek.
 */
void            bold(const char tekst[]);

/* Oblicza koszt połączenia, ilość impulsów, etc. */
void            oblicz(u_int data, const u_int koniec, u_int *sumimpulsy, double *sumkoszt, char *flagi, const size_t rozm);

/*
 * Wyświetla numer wersji programu.
 */
void            version(void);

/*
 * Wyświelta listę możliwych do użycia argumentów.
 */
void            usage(void);

/*
 * Kolejne wywołania zwracają zawartość kolejnych wierszy z pliku fd1. Jeżeli
 * fd1 się skończy (EOF), to zwracana jest zawartość z pliku fd2. Dopiero gdy
 * fd2 się skończy jest zwracany NULL. Odczytane linie są zapisywane do bufora.
 * Jednorazowo jest odczytywanych maksymalnie 'lim' znaków.
 * fd1 musi istnieć. Jeżeli fd2 nie istnieje, to fd2 musi być równe NULL.
 */
char           *pobierz(FILE * fd1, FILE * fd2, char *bufor, const int lim);

/*
 * Sprawdza czy dany 'text' zawiera dozwolone znaki. Lista dozwolonych znaków
 * jest zapisana w tablicy allowed.
 */
u_int             validate(const char text[], const char *allowed);

/*
 * Oblicza od którego znaku w podanym buforze zaczyna się następna data.
 * Umożliwia poprawne odczytanie dat z różnych stref czasowych, gdyż nażwy tych
 * stref mają różną ilość znaków.
 */
u_int		  next_date_start(const char date[]);

int
main(int argc, char *argv[])
{
	/*
	 * Lista dozwolonych znaków w wartościach liczbowych.
	 */
	char            allowed_num[] =
	{
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '\0'
	};

	/*
	 * Lista znaków dozwolonych w nazwach plików, katalogów.
	 */
	char            allowed_path[] =
	{
		' ', '!', '&', '\'', '(', ')', '+', ',', '-', '.', '/', '0',
		'1', '2', '3', '4', '5', '6', '7', '8', '9', ':', '?', '@',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
		'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', '[', ']', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
		's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '}', '~', '\0'
	};

	/*
	 * Lista znaków dozwolonych w stringach określających datę.
	 */
	char            allowed_date[] =
	{
		' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
		'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
		'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '\n', '\0'
	};

	FILE           *fd1, *fd2;

	/*
	 * Do przechowywania różnych "śmieci".
	 */
	char            c, bufor[200];
	int		i = 0;

	/*
	 * Do przechowywania 'R', 'Z' lub 'P' ;-).
	 */
	char		status;

	/*
	 * Do przechowywania ścieżki do pliku z listą połączeń.
	 */
	char           *file_log;

	/*
	 * Do przechowywania nazwy pliku z listą połączeń.
	 */
	char	       *file_log_suffix;

	/*
	 * Domyślna nazwa pliku z listą połączeń.
	 */
	char            file_log_suffix_default[] = ".trf";

	/*
	 * Do przechowywania nazwy pliku z informacją o aktualnym połączeniu.
	 */
	char	       *file_current;

	/*
	 * Końcówka nazwy pliku z informacją o aktualnym połączeniu.
	 */
	char            file_current_suffix[] = ".current";
	
	/*
	 * Do przechowywania zawartości TRFLIMIT.
	 */
	char	       *trflimit;
	
	/*
	 * Do przechowywania zawartości HOME.
	 */
	char           *home;

	/*
	 * Do przechowywania 'flag' obrazujących przebieg klasyfikacji
	 * połączenia.
	 */
	char	       *flagi;

	/*
	 * Sumaryczny koszt połączeń w danym miesiącu.
	 */
	double          sumkoszt = 0.00;

	/*
	 * Długość ryczałtu w _sekundach_.
	 * 81000 sekund = 22,5 godziny.
	 */
	int             limit = 81000;

	/*
	 * Numer bieżącego miesiąca.
	 */
	u_int           month = 0;

	/*
	 * Długość danego połączenia (w sekundach).
	 */
	int             dlugosc;

	/*
	 * Sumaryczna długość połączeń.
	 */
	u_int		sumdlugosc = 0;

	/*
	 * Przed zmianą wartości 'sumdlugosc', aktualna wartość jest zapisywana
	 * do 'poprzsumdlugosc'.
	 */
	u_int		poprzsumdlugosc = 0;

	/*
	 * Ilość sekund jaka upłynęła od Epoch w momencie rozpoczęcia
	 * połączenia.
	 */
	int             sekundyS;

	/*
	 * Ilość sekund jaka upłynęła od Epoch w momencie zakończenia
	 * połączenia.
	 */
	int		sekundyK;

	/*
	 * Do przechowywania ilości sekund, minut i godzin.
	 */	
	u_int             hou, min, sec;

	/*
	 * Rozmiary różnych char'ów. Nazwy odpowiedników poprzedzone są 'rozm_'.
	 */
	size_t          rozm_home, rozm_file_log_suffix, rozm_file_log;
	size_t          rozm_file_current, rozm_trflimit, rozm_flagi;

	/*
	 * Struktury do przechowywania dat: rozpoczęcia połączenia, końca
	 * połączenia i aktualnej daty.
	 */
	struct tm       stime, etime, atime;

	/*
	 * Przechowuje datę w postaci liczby sekund od Epoch.
	 */
	time_t          atimet;

	/*
	 * Liczba linii w pliku ~/.trf i liczba linii odczytanych.
	 */
	u_int	    linie = 0, odczytane_linie = 0;

	/*
	 * Sumaryczna liczba impulsów.
	 */
	u_int		sumimpulsy = 0;

	/*
	 * Liczba połączeń dokonanych w danym miesiącu.
	 */
	u_int		liczbapolaczen = 0;

	/*
	 * Z którego pliku mamy czytać informacje o połączeniach.
	 * 1 -> czytaj z ~/.trf
	 * 2 -> czytaj z ~/.trf.current
	 */
	plik = 1;

	/*
	 * Rezerwujemy pamięć dla 'flagi'.
	 */
	rozm_flagi = 9;
	flagi = malloc(rozm_flagi);

	/*
	 * Inicjujemy strukturę 'atimet' aktualną datą.
	 */
	(void)time(&atimet);

	/*
	 * Do atime zapisujemy aktualną datę.
	 */
	atime = *localtime(&atimet);

	/*
	 * Do month zapisujemy numer bieżącego miesiąca.
	 */
	month = (u_int)atime.tm_mon;

	/*
	 * Jeżeli zmienna TRFLIMIT jest określona, to ...
	 */
	if (getenv("TRFLIMIT") != NULL) {
		/*
		 * Sprawdzamy jaki rozmiar ma TRFLIMIT.
		 */
		rozm_trflimit = strlen(getenv("TRFLIMIT")) + 1;
		/*
		 * I właśnie taką ilość miejsca przygotowujemy do przechowywania
		 * wartości.
		 */
		trflimit = malloc(rozm_trflimit);
		/*
		 * Jeżeli malloc się nie popisał, to wychodzimy.
		 */
		if (!trflimit) {
			fprintf(stderr, "Nie udało się zalokować pamięci dla 'trflimit'.\n");
			exit(1);
		}
		/*
		 * Kopiujemy wartość TRFLIMIT do 'trflimit'.
		 */
		strncpy(trflimit, getenv("TRFLIMIT"), rozm_trflimit);
		trflimit[rozm_trflimit] = '\0';

		/*
		 * Jeżeli w 'trflimit' są jakieś paskudztwa to wychodzimy.
		 */
		if (validate(trflimit, allowed_num) == 1) {
			fprintf(stderr, "Zmienna TRFLIMIT zawiera nieprawidłowe znaki. Dozwolone są tylko cyfry.\n");
			exit(1);
		}
		/*
		 * Jeżeli wszysko jest O.K., to konwertujemy wartość ze stringa
		 * na int.
		 */
		limit = atoi(trflimit);
		/*
		 * Zwalniamy już niepotrzebną pamięć.
		 */
		free(trflimit);
	}

	/*
	 * Sprawdzamy czy limit ma dodatnią wartość.
	 */
	if (limit < 0) {
		fprintf(stderr, "Wartość limitu musi być większa od zera.\n");
		exit(1);
	}

	/*
	 * Aby określić położenia pliku z logiem musimy znać wartość zmiennej
	 * HOME.
	 */
	if (getenv("HOME") == NULL) {
		fprintf(stderr, "Określ zmienną środowiska HOME.\n");
		exit(1);
	}

	/*
	 * Sprawdzamy jaki rozmiar ma HOME.
	 */
	rozm_home = strlen(getenv("HOME")) + 1;

	/*
	 * Alokujemy pamięć dla home, etc...
	 */
	home = malloc(rozm_home);
	if (!home) {
		fprintf(stderr, "Nie udało się zalokować pamięci dla 'home'.\n");
		exit(1);
	}

	/*
	 * Zapisujemy wartość HOME do 'home'.
	 */
	strncpy(home, getenv("HOME"), rozm_home);
	home[rozm_home] = '\0';

	/*
	 * Sprawdzamy czy 'home' zawiera tylko dozwolone znaki.
	 */
	if (validate(home, allowed_path) == 1) {
		fprintf(stderr, "Zmienna HOME zawiera niedzowolone znaki.\n");
		exit(1);
	}

	/*
	 * Jeżeli zmienna TRFFILE jest określona, to...
	 */
	if (getenv("TRFFILE") != NULL) {
		/*
		 * Sprawdzamy jej rozmiar, alokujemy dla niej pamięć, zapisujemy
		 * warotść do 'file_log_suffix' i sprawdzamy czy zawiera tylko
		 * dozwolone znaki.
		 */
		rozm_file_log_suffix = strlen(getenv("TRFFILE")) + 1;
		file_log_suffix = malloc(rozm_file_log_suffix);
		if (!file_log_suffix) {
			fprintf(stderr, "Nie udało się zalokować pamięci dla 'file_log_suffix'.\n");
			exit(1);
		}
		strncpy(file_log_suffix, getenv("TRFFILE"), rozm_file_log_suffix);
		file_log_suffix[rozm_file_log_suffix] = '\0';
		if (validate(file_log_suffix, allowed_path) == 1) {
			fprintf(stderr, "Zmienna TRFFILE zawiera niedozwolone znaki.\n");
			exit(1);
		}
	} else {
		/*
		 * Jeżeli zmienna TRFFILE nie jest określona, to używamy wartości
		 * domyślnych...
		 */
		rozm_file_log_suffix = strlen(file_log_suffix_default) + 1;
		file_log_suffix = malloc(rozm_file_log_suffix);
		if (!file_log_suffix) {
			fprintf(stderr, "Nie udało się zalokować pamięci dla 'file_log_suffix' (2).\n");
			exit(1);
		}
		strncpy(file_log_suffix, file_log_suffix_default, rozm_file_log_suffix);
		file_log_suffix[rozm_file_log_suffix] = '\0';
	}

	/*
	 * Sprawdzamy jaki rozmiar mam mieć 'file_log' (rozmiar sufiksu +
	 * rozmiar 'home').
	 */
	rozm_file_log = rozm_file_log_suffix + rozm_home;

	/*
	 * Alokujemy miejsce dla 'file_log'.
	 */
	file_log = malloc(rozm_file_log);
	if (!file_log) {
		fprintf(stderr, "Nie udało się zalokować pamięci dla 'file_log'.\n");
		exit(1);
	}

	/*
	 * Zapisujemy w 'file_log' ostateczną postać ścieżki dostępu do pliku z
	 * logiem: 'home' + '/' + sufiks.
	 */
	strncpy(file_log, home, rozm_file_log);
	file_log[rozm_file_log] = '\0';
	strncat(file_log, "/", rozm_file_log - strlen(file_log));
	strncat(file_log, file_log_suffix, rozm_file_log - strlen(file_log));

	/*
	 * Zwalniamy pamięć zajmowaną przez 'home' i 'file_log_suffix'. Te
	 * zmienne nie będą już nam potrzebne.
	 */
	free(home);
	free(file_log_suffix);

	/*
	 * Określamy rozmiar 'file_current': rozmiar 'file_log' - znak
	 * terminacji ('\0') + rozmiar 'current_suffix'.
	 */
	rozm_file_current = rozm_file_log - 1 + strlen(file_current_suffix);

	/*
	 * Alokujemy pamięć dla 'file_current'.
	 */
	file_current = malloc(rozm_file_current);
	if (!file_current) {
		fprintf(stderr, "Nie udało się zalokować pamięci dla 'file_current'.\n");
		exit(1);
	}

	/*
	 * W 'file_current' zapisujemy ostateczną postać ścieżki dostępu do
	 * pliku z informacją o aktualnym połączeniu.
	 */
	strncpy(file_current, file_log, rozm_file_current);
	file_current[rozm_file_current] = '\0';
	strncat(file_current, file_current_suffix, rozm_file_current - strlen(file_current));

	/*
	 * Sprawdzamy z jakimi argumentami został uruchomiony program.
	 */
	while ((i = getopt(argc, argv, "Vm:h")) != -1) {
		switch (i) {
		case 'V':
			/*
			 * Jeżeli jednym z argumentów jest '-V', to wyświetlamy
			 * informacje o wersji programu i wychodzimy.
			 */
			version();
			exit(0);
		case 'm':
			/*
			 * Jeżeli argumentem jest '-m N', to ...
			 * ... sprawdzamy czy podana liczba składa się tylko z
			 * cyfr.
			 */
			if (validate(optarg, allowed_num) == 1) {
				fprintf(stderr, "Podano zły numer miesiąca. Dozwolone są tylko cyfry.\n");
				exit(1);
			}
			/*
			 * Konwertujemy stringa do int.
			 */
			month = (u_int)atoi(optarg);
			month--;
			/*
			 * Sprawdzamy czy wartość 'month' znajduje się w
			 * dozwolonym przedziale.
			 */
			if (month > 11) {
				fprintf(stderr, "Podano zły numer miesiąca.\n");
				exit(1);
			}
			break;
		case '?':
		default:
			/*
			 * Jeżeli program został wywołany z dowolną inną opcją,
			 * to wyświetlamy informacje o możliwych argumentach i
			 * wychodzimy.
			 */
			usage();
			exit(1);
		}
	}

	/*
	 * Jeżeli plik z informacjami o połączeniach nie istnieje, to
	 * wychodzmimy.
	 */
	if ((fd1 = fopen(file_log, "r")) == NULL) {
		fprintf(stderr, "Brak pliku z logami.\n");
		exit(1);
	}

	/*
	 * Sprawdzamy ile linii ma plik z logami.
	 */
	while ((c = getc(fd1)) != EOF) {
		if (c == '\n')
			linie++;
	}

	/* ...i zamykamy go. */
	(void)fclose(fd1);

	/* Wyświetlamy nagłówek ramki. */
	bold(",--------------------------------+----------+----------+----------+-----------.\n");
	bold("|");
	printf(" początek - koniec połączenia   ");
	bold("|");
	printf(" długość  ");
	bold("|");
	printf(" impulsy  ");
	bold("|");
	printf(" koszt    ");
	bold("|");
	printf(" flagi     ");
	bold("|\n");
	bold("+--------------------------------+----------+----------+----------+-----------+\n");

	/*
	 * Otwieramy pliki ~/.trf i ~/.trf.current i zwalniamy pamięć zajmowaną
	 * przez stringi zawierające nazwy tych plików.
	 */
	fd1 = fopen(file_log, "r");
	free(file_log);
	fd2 = fopen(file_current, "r");
	free(file_current);

	/*
	 * Dopóki będziemy mogli pobrać jakieś dane z plików, to...
	 */
	while (pobierz(fd1, fd2, bufor, 63) != NULL) {
		/*
		 * Zwiększamy liczbę odczytanych linii.
		 */
		odczytane_linie++;

		/*
		 * Sprawdzamy, czy bufor zawiera tylko dozwolone znaki.
		 */
		if (validate(bufor, allowed_date) == 1) {
			fprintf(stderr, NORMAL);
			fprintf(stderr, "Rekord znajdujący się w %u linii zawiera nieprawidłowe znaki. Dozwolone są tylko cyfry, małe i duże litery oraz dwukropek.\n", odczytane_linie);
			exit(1);
		}

		/*
		 * Do struktury 'stime' zapisujemy pierwszą datę odczytaną z bufora.
		 */
		if (strptime(bufor, TIME_CHAR, &stime) == NULL) {
			fprintf(stderr, NORMAL);
			fprintf(stderr, "Błąd podczas przetwarzania rekordu z %u linii.\n", odczytane_linie);
			exit(1);
		}

		/*
		 * Do struktury 'etime' zapisujemy drugą datę znajdującą się w
		 * buforze.
		 */
		if (strptime(&(bufor[(int) next_date_start(bufor)]), TIME_CHAR, &etime) == NULL) {
			fprintf(stderr, NORMAL);
			fprintf(stderr, "Błąd podczas przetwarzania rekordu z %u linii.\n", odczytane_linie);
			exit(1);
		}

		/*
		 * Jeżeli miesiąc w którym się rozpoczęło połączenie, jest inny
		 * niż zawarty w zmiennej 'mont', to pomijamy to połączenie.
		 */
		if (month != (u_int)stime.tm_mon)
			continue;

		/*
		 * Zwiększamy liczbę połączeń, które zostały dokonane w danym
		 * miesiącu.
		 */
		liczbapolaczen++;

		/*
		 * Wyświetlamy informacje o czasie rozpoczęcia i zakończenia
		 * połączenia.
		 */
		bold("|");
		printf(" %i/%2i/%2i ", stime.tm_year + 1900, stime.tm_mon + 1, stime.tm_mday);

		if (stime.tm_hour < 10)
			printf("0");
		printf("%i:", stime.tm_hour);
		if (stime.tm_min < 10)
			printf("0");
		printf("%i:", stime.tm_min);
		if (stime.tm_sec < 10)
			printf("0");
		printf("%i - ", stime.tm_sec);

		if (etime.tm_hour < 10)
			printf("0");
		printf("%i:", etime.tm_hour);
		if (etime.tm_min < 10)
			printf("0");
		printf("%i:", etime.tm_min);
		if (etime.tm_sec < 10)
			printf("0");
		printf("%i ", etime.tm_sec);

		bold("|");

		/*
		 * Datę ze struktury 'stime' zamieniamy na liczę sekund od
		 * Epoch i zapisujemy w 'sekundyS'.
		 */
		if (strftime(bufor, sizeof(bufor) - 1, "%s", &stime) == 0) {
			fprintf(stderr, "\nWystąpił błąd funkcji strftime() podczas przetwarzania rekordu z linii numer %u.\n", odczytane_linie);
			exit(1);
		}
		sekundyS = atoi(bufor);

		/*
		 * Jeżeli funkcja strftime() coś namieszała, to wychodzimy...
		 */
		if (sekundyS <= 0) {
			fprintf(stderr, "\nFunkcja strftime zwróciła złą liczbę sekund. Zgłoś ten błąd.\n");
			exit(1);
		}

		/*
		 * Datę ze struktury 'etime' zamieniamy na liczbę sekund od
		 * Epoch i zapisujemy w 'sekundyK'.
		 */
		if (strftime(bufor, sizeof(bufor) - 1, "%s", &etime) == 0) {
			fprintf(stderr, "\nWystąpił błąd funkcji strftime() podczas przetwarzania rekordu z linii numer %u.\n", odczytane_linie);
			exit(1);
		}
		sekundyK = atoi(bufor);

		if (sekundyK <= 0) {
			fprintf(stderr, "\nFunkcja strftime() zwróciła złą liczbę sekund.\n");
			exit(1);
		}

		/*
		 * Obliczamy długość połączenia w sekundach.
		 */
		dlugosc = sekundyK - sekundyS;

		/*
		 * Jeżeli długość połączenia jest ujemna, to coś musi być nie
		 * tak.
		 */
		if (dlugosc < 0) {
			fprintf(stdout, "\n");
			fprintf(stderr, NORMAL "Błąd. Ujemna długość połączenia.\n");
			exit(1);
		}

		/*
		 * Dotychczasową wartość 'sumdlugosc' zapisujemy do
		 * 'poprzsumdlugosc', a 'sumdlugosc' zwiększamy o czas
		 * aktualnego połączenia.
		 */
		poprzsumdlugosc = sumdlugosc;
		sumdlugosc += (u_int)dlugosc;

		/*
		 * Długość połączenia w sekundach zamieniamy na normalną datę i
		 * wyświetlamy.
		 */
		sec = (u_int)dlugosc;
		min = 0;
		hou = 0;

		while (sec >= 60) {
			min = min + (sec / 60);
			sec = sec % 60;
		}

		while (min >= 60) {
			hou = hou + (min / 60);
			min = min % 60;
		}

		if (hou < 10) {
			printf(" 0");
			printf("%u:", hou);
		} else {
			printf("%2u:", hou);
		}

		if (min < 10)
			printf("0");
		printf("%u:", min);

		if (sec < 10)
			printf("0");
		printf("%u ", sec);

		bold("|");

		if ((int)sumdlugosc < limit && (int)poprzsumdlugosc < limit) {
			/*
			 * Jeżeli nie przekroczyliśmy jeszcze czasu ryczałtu, to
			 * nie musimy obliczać nic więcej.
			 */
			printf("          ");
			bold("|");
			printf("          ");
			bold("|");
			status = 'R';
		} else if ((int)poprzsumdlugosc < limit && (int)sumdlugosc > limit) {
			/*
			 * Jeżeli połączenie zostało rozpoczęte w czasie trwania
			 * ryczałtu, a zakończone jak przekroczyliśmy limit, to
			 * obliczamy ile zapłacimy za czas połączenia dokonanego
			 * poza ryczałtem.
			 */
			dlugosc = (int)sumdlugosc - (int)limit;
			oblicz((u_int)sekundyS + (u_int)limit - poprzsumdlugosc, (u_int)sekundyK, &sumimpulsy, &sumkoszt, flagi, rozm_flagi);
			bold("|");
			status = 'Z';
		} else if ((int)poprzsumdlugosc >= limit && (int)sumdlugosc >= limit) {
			/*
			 * Jeżeli połączenie zostało dokonane całkowicie poza
			 * ryczałtem, to tylko obliczamy jego koszt.
			 */
			oblicz((u_int)sekundyS, (u_int)sekundyK, &sumimpulsy, &sumkoszt, flagi, rozm_flagi);
			bold("|");
			status = 'P';
		} else {
			/*
			 * Ten fragment nie powinien zostać wykonany.
			 */
			fprintf(stderr, "\nBłąd. Połączenie nie zostało poprawnie zakwalifikowane.\n");
			exit(1);
		}

		/*
		 * Jeżeli odczytaliśmy o jedną linię więcej niż liczba linii
		 * znajdujących się w pliku ~/.trf, to znaczy, że ta linia jest
		 * z ~/.trf.current.
		 */
		if (odczytane_linie == (linie + 1)) {
			printf("%8s%cA", flagi, status);
		} else {
			printf("%9s%c", flagi, status);
		}
		bold(" |\n");
	}

	/*
	 * Zamykamy niepotrzebne już pliki.
	 */
	(void)fclose(fd1);
	if (fd2) {
		(void)fclose(fd2);
	}

	/*
	 * Zwalniamy pamięć zajmowaną przez 'flagi'.
	 */
	free(flagi);

	/*
	 * Jeżeli liczba połączeń jest równa zero, to wyświetlamy tylko
	 * niezbędną informację i wychodzimy.
	 */
	if (liczbapolaczen == 0) {
		bold("|");
		printf(" Brak dokonanych połączeń                                                     ");
		bold("|\n");
		bold("`--------------------------------+----------+----------+----------+-----------'\n");
		return 0;
	}

	/*
	 * Wyświetlamy dolną część tabeli.
	 */
	bold("`--------------------------------+----------+----------+----------+-----------'\n");
	printf(" R - ryczałt / Z - część w ryczałcie, część poza / P - poza ryczałtem\n");
	printf(" d - droższa taryfa / t - tańsza / w - weekend / A - aktualne połączenie\n");
	printf(" ! - za duża liczba flag, nie wszystkie się zmieściły\n");

	/*
	 * Wyświetlamy sumaryczny czas dokonanych połączeń i inne "bajery".
	 */
	sec = sumdlugosc;
	min = 0;
	hou = 0;
	while (sec >= 60) {
		min = min + (sec / 60);
		sec = sec % 60;
	}
	while (min >= 60) {
		hou = hou + (min / 60);
		min = min % 60;
	}
	printf("\nSumaryczna długość połączeń:\t");
	if (hou < 10)
		printf("0");
	printf("%u:", hou);
	if (min < 10)
		printf("0");
	printf("%u:", min);
	if (sec < 10)
		printf("0");
	printf("%u\n", sec);

	printf("Sumaryczny koszt połączeń:\t%.2f\n", sumkoszt);
	printf("Sumaryczna liczba impulsów:\t%u\n", sumimpulsy);
	printf("Liczba dokonanych połączeń:\t%u\n", liczbapolaczen);

	/*
	 * Koniec. :-)
	 */
	return 0;
}

void
bold(const char tekst[])
{
	/*
	 * Trzeba coś tutaj tłumaczyć?
	 */
	printf(BOLD "%s" NORMAL, tekst);
}

void
oblicz(u_int data, u_int koniec, u_int *sumimpulsy, double *sumkoszt, char *flagi, const size_t rozm)
{
	/*
	 * Struktura do przechowywania daty.
	 */
	struct tm       atime;

	/*
	 * Do przechowywania pojedyńczej flagi.
	 */
	char           *flaga;

	/*
	 * Taki bufor...
	 */
	char            bufor[100];

	/*
	 * Liczba impulsów.
	 */
	u_int	    impulsy = 0;

	/*
	 * Koszt połączenia.
	 */
	double          koszt = 0.00;

	/*
	 * Cena impulsu poza ryczałtem.
	 */
	const double    cenaimpulsu = 0.35;

	/*
	 * Długość impulsu w droższej taryfie.
	 */
	const u_int	dlimpulsudrozej = 180;

	/*
	 * Długość impulsu w tańszej taryfie.
	 */
	const u_int	dlimpulsutaniej = 360;

	/*
	 * Zerujemy zawartość tablicy 'flagi'.
	 */
	bzero(flagi, rozm);

	/*
	 * Dopóki 'data' będzie mniejsza lub równa 'koniec' (;-) strasznie
	 * trudne).
	 */
	while (data <= koniec) {
		/*
		 * Do bufora zapisujemy datę.
		 */
		snprintf(bufor, sizeof(bufor) - 1, "%u", data);

		/*
		 * I zapisujemy ją w strukturze 'atime'.
		 */
		if (strptime(bufor, "%s", &atime) == NULL) {
			fprintf(stderr, "oblicz(): błąd strptime()");
			exit(1);
		}

		/*
		 * Jeżeli dzień to sobota lub niedziela, czyli weekend, czyli
		 * tańsza taryfa, to...
		 */
		if (atime.tm_wday == 0 || atime.tm_wday == 6) {
			/*
			 * Jako flagę ustawiamy 'w', zwiększamy liczbę impulsów,
			 * zwiększamy cenę połączenia, zwiększamy wartość 'daty'
			 * o odpowiednią długość impulsu.
			 */
			flaga = "w";
			impulsy++;
			koszt += cenaimpulsu;
			data += dlimpulsutaniej;
		} else {
			/*
			 * Jeżeli to nie jest weekend...
			 */
			if (atime.tm_hour >= 8 && atime.tm_hour < 18) {
				/*
				 * I do tego to jest między 8:00 a 18:00, to
				 * musi to być droższa taryfa. Resztę już
				 * znacie.
				 */
				flaga = "d";
				impulsy++;
				koszt += cenaimpulsu;
				data += dlimpulsudrozej;
			} else if (atime.tm_hour < 8 || atime.tm_hour >= 18) {
				/*
				 * Koniec końców, jeżeli to nie jest weekend i
				 * jest między 18:00 a 8:00, to musi to być
				 * tańsza taryfa.
				 */
				flaga = "t";
				impulsy++;
				koszt += cenaimpulsu;
				data += dlimpulsutaniej;
			} else {
				/*
				 * Jakiś błąd w algorytmie?
				 */
				fprintf(stderr, "\nBłąd. Nie dopasowano taryfy.\n");
				exit(1);
			}
		}

		/*
		 * Mała "kompresja" flag. Jezeli 'flaga' znajduje się już na
		 * końcu 'flagi', to nie zapisujemy jej n-ty raz. Jeżeli w
		 * 'flagi' nie ma jeszcze flag, to dopisujemy ją.
		 */
		if (flagi[0] == '\0' || flagi[strlen(flagi) - 1] != *flaga) {
			/*
			 * Jeżeli ostatni i przedostatni znak w 'flagi' to '\0',
			 * to znaczy, że zmieści się tam co najmniej jeszcze
			 * jedna flaga. Dopisujemy.
			 */
			if (flagi[rozm - 1] == '\0' && flagi[rozm - 2] == '\0') {
				strcat(flagi, flaga);
			} else {
				/*
				 * Jeżeli przedostatni znak w 'flagi' to nie
				 * '\0', to zamieniamy go na '!'. Ten fragment
				 * zostanie wywołany tylko wtedy, gdy we 'flagi'
				 * nie zmieści się już żaden znak.
				 */
				flagi[rozm - 2] = '!';
			}
		}
	}

	/*
	 * Wyśiwetlamy liczbę impulsów i koszt połączenia.
	 */
	printf("%9u ", impulsy);
	bold("|");
	printf("%9.2f ", koszt);

	/*
	 * Zwiększamy warotści odpowiednich ziennych.
	 */
	*sumimpulsy += impulsy;
	*sumkoszt += koszt;
}

void
version(void)
{
	/*
	 * Hmmm, to pewnie wyświetla numer wersji programu, ale do końca pewien
	 * nie jestem ;-).
	 */
	printf("Trf " VERSION " by Maciej Korzeń <eaquer@ceti.pl>\n");
}

void
usage(void)
{
	/*
	 * Wyświetla listę argumentów.
	 */
	printf("Użycie: trfs [-V] [-m miesiąc]\n"
	       "\t-V\t\t\twyświetla wersję programu\n"
	       "\t-m miesiąc\t\tnumer miesiąca (od 1 do 12)\n");
}

char           *
pobierz(FILE * fd1, FILE * fd2, char *bufor, const int lim)
{
	/*
	 * Czytamy z pierwszego pliku.
	 */
	if (plik == 1) {
		if (fgets(bufor, lim, fd1) != NULL) {
			/*
			 * Jeżeli funkcji fgets() udało się odczytać "coś", to
			 * zwracamy to.
			 */
			return (bufor);
		} else {
			/*
			 * W przeciwnym wypadku będziemy czytać z drugiego
			 * pliku.
			 */
			plik = 2;
		}
	}

	/*
	 * Czytamy z drugiego pliku.
	 */
	if (plik == 2) {
		/*
		 * Jeżeli możemy odczytać z tego pliku, to...
		 */
		if (fd2 != NULL) {
			/*
			 * Zwracamy to samo, co funkcja fgets().
			 */
			return (fgets(bufor, lim, fd2));
		} else {
			/*
			 * Jeżeli nie możemy odczytać z tego pliku, to zwracamy
			 * NULL.
			 */
			return (NULL);
		}
	}

	/*
	 * Ten fragment nie powinien być wykonany.
	 */
	fprintf(stderr, "Błąd w funkcji pobierz().\n");
	exit(1);
}

u_int 
validate(const char text[], const char *allowed)
{
	u_int	    i;

	/*
	 * Dla każdego znaku z textu[] sprawdzamy czy znajduje się on w
	 * tablicy allowed_char. Jeżeli nie, to robimy return 0 i do
	 * wiedzenia.
	 */
	for (i = 0; text[i] != '\0'; i++) {
		if (strchr(allowed, text[i]) == NULL) {
			return 1;
		}
	}
	return 0;
}

u_int
next_date_start(const char date[]) {
		/*
		 * Liczba odczytanych odstępów (odstępt = nie-spacja + spacja).
		 */
		u_int spaces;

		/*
		 * Znak od którego rozpoczyna się następna data.
		 */
		u_int date_start;

		for (spaces = 0, date_start = 0 ; spaces < 6 ; date_start++) {
			if ( date[date_start] == ' ' && date[date_start - 1] != ' ') {
				spaces++;
			}
		}

		return date_start;
}

