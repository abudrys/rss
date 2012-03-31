/*
 * File:   help.h
 * Author: Adrian Jamróz
 */
     
#ifndef	__HELP_H
#define	__HELP_H

struct usage_data
{
	const char *ud_cmd;
	const char *ud_description;
	const char *ud_syntax;
	const char *ud_usage;
};

static const struct usage_data usage_texts[] = {
	{ 
	        "add", "dodaje nowe źródło RSS", 
	        "add <nazwa_źródła> <http://adres_źródła>", 
	        "Polecenie 'add' dodaje nowy kanał RSS do listy skonfigurowanych\n"
	        "kanałów. Pierwszym argumentem jest unikalna nazwa-identyfikator\n"
	        "źródła, a drugim URL do pliku *.rss lub *.xml, zawierającym dane\n"
	        "wybranego kanału.\n"
	},
	{
	        "remove", "usuwa istniejące źródło RSS",
	        "remove <nazwa_źródła>",
	        "Polecenie 'remove' usuwa uprzednio skonfigurowany kanał RSS oraz\n"
	        "wszystkie dotychczas zgromadzone wiadomości z tego kanału. Operacja\n"
	        "ta jest nieodwracalna.\n"
	},
	{
	        "list", "wypisuje skonfigurowane źródla RSS",
	        "list",
	        "Polecenie 'list' wypisuje listę skonfigurowanych źródeł RSS wraz\n"
	        "z ich URL-em oraz ilością wiadomości z bazie danych.\n"
	},
	{
	        "update", "pobiera nowe wiadomości z wybranego lub wszystkich źródeł RSS",
	        "update [nazwa_źródła]",
	        "Polecenie 'update' pobiera nowe wiadomości ze źródła podanego w pierwszym\n"
	        "argumencie (jeśli podane), lub ze wszystkich skonfigurowanych źródeł\n"
	        "(jeśli uruchomione bez dodatkowych argumentów).\n"
	},
	{
	        "view", "wyświetla wiadomości ze źródeł RSS",
	        "view [parametry]",
	        "Polecenie 'view' wyświetla zgromadzone w bazie danych wiadomości. Uruchomione\n"
	        "bez argumentów, wyświetli wiadomości ze wszystkich skonfigurowanych źródeł z\n"
	        "okresu od ostatniego pobierania wiadomości do teraz.\n"
	        "Opcjonalnie, można podać jeden lub kilka parametrów precyzujących wyszukiwanie\n"
	        "wiadomości. Parametry oddzielamy spacją. Dostępne parametry:\n"
	        "\tfeed <nazwa_źródła> -- wybiera wiadomości tylko ze źródła o podanej nazwie.\n"
	        "\tolder <data/czas> -- wybiera wiadomości starsze niż...\n"
	        "\tnewer <data/czas> -- wybiera wiadomości nowsze niż...\n"
	        "\tall -- wybiera wszystkie wiadomości. Jednocześnie unieważnia wcześniej podane parametry.\n"
	        "\n"
	        "\tParametr <data/czas> dla parameteru 'older' oraz 'newer' może być podany w formacie:\n"
	        "\tliczba(h|m|d), np.: 12h, 2d lub 60m.\n"
	},
	{
	        "flush", "usuwa najstarsze wiadomości zgromadzone w bazie danych",
	        "flush <data/czas>",
	        "Polecenie 'flush' usuwa zgromadzone w bazie danych wiadomości ze źródeł RSS\n"
	        "starsze niż podany okres czasu. Parametrem tej funkcji jest liczba minut, godzin\n"
	        "lub dni w formacie: liczba(m|h|d), np: 12h, 2d lub 60m.\n"
	},
	{
	        "set", "wyświetla lub ustawia zawartość wewnętrznej zmiennej",
	        "set [nazwa_zmiennej] [wartość]",
	        "Polecenie 'set' wyświetla lub ustawia wartość wewnętrnej zmiennej Może być.\n"
	        "wywołane bez dodatkowych argumentów, z jednym lub z dwoma argumentami.\n"
		"W tym pierwszym przypadku, wyświetli listę wszystkich zmiennych wraz z ich\n"
		"wartościami. W drugim przypadku, gdy podana jest nazwa zmiennej, wyświetli\n"
		"tylko tę zmienną i jej wartość. Trzeci przypadek, w którym podajemy nazwę\n"
		"zmiennej i jej wartość, ustawia wartość podanej zmiennej.\n"
	},
	{
		"help", "wyświetla treść pomocy",
		"help [polecenie]",
		"Polecenie 'help' wyświetla treść pomocy. W pierwszym argumencie można podać\n"
		"nazwę polecenia którego pomoc ma dotyczyć.\n"
	},
	{
		"about", "wyświetla informacje o autorze programu",
		"about",
		"Polecenie 'about' wyświetla nieco zdawkowych informacji na temat autora i\n"
		"w skrócie wyjaśnia, z jakich pobudek on powstał.\n"
	},
	{
		"exit", "kończy pracę programu",
		"exit",
		"Polecenie 'exit' kończy pracę programu.\n"
	},
};

#endif	/* __HELP_H */
