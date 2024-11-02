# Mini Shell en C

Ce projet est un interpréteur de commandes en C, conçu pour simuler le comportement d'un shell Unix simple. Il a été développé dans le cadre de ma licence en informatique, dans un cours sur les systèmes d'exploitation. Ce mini shell prend en charge plusieurs commandes de base, la gestion des processus, et des fonctionnalités de redirection et de pipes entre commandes.

## Fonctionnalités

- **Commandes intégrées** : Le shell prend en charge plusieurs commandes intégrées, telles que `cd`, `exit`, `pwd`, et `ls`.
- **Exécution de commandes externes** : Si une commande n'est pas intégrée, le shell utilise `execvp` pour exécuter des commandes du système.
- **Pipes** : Le shell supporte les pipes entre deux commandes (`commande1 | commande2`).
- **Gestion des processus** : Le shell utilise `fork` pour exécuter des processus enfants et `wait` pour gérer leur terminaison.
- **Gestion des signaux** : Le shell ignore certains signaux (comme `SIGINT` pour éviter l'interruption du shell principal) et restaure les comportements par défaut pour les processus enfants.
- **Parsing des commandes** : Utilisation de fonctions pour analyser et interpréter les commandes entrées par l'utilisateur.

## Structure du Code

### Principales Structures et Fonctions

- **Structures**
  - `struct builtin` : Représente une commande intégrée avec un nom et un pointeur de fonction.
  - `struct command` : Contient les arguments et le nombre d'arguments pour une commande à exécuter.

- **Fonctions**
  - `int count_args(const char *s)` : Compte les arguments dans une chaîne de caractères.
  - `struct command *parse_cmd(const char *buf, int len)` : Analyse une commande utilisateur et la convertit en une structure `command`.
  - `int find_builtin(char *cmd)` : Recherche une commande dans la liste des commandes intégrées.
  - `void exec_cmd(struct command *cmd)` : Exécute une commande en créant un processus enfant.
  - `void exec_piped_cmds(struct command *cmd1, struct command *cmd2)` : Exécute deux commandes avec un pipe.
  - **Commandes intégrées** : `builtin_exit`, `builtin_cd`, `outil_pwd`, `ls`.

### Utilisation

Pour démarrer le shell, compilez le code en utilisant `gcc` :

```bash
gcc -o mini_shell mini_shell.c
./mini_shell



