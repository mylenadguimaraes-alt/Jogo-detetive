#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define Maxstring 100
#define HASH_SIZE 101

// ---------- Structs ----------

// Sala (árvore binária simples)
typedef struct Sala {
    char nome[Maxstring];
    struct Sala *esquerda;
    struct Sala *direita;
    struct Sala *pai; // para permitir voltar
} Sala;

// BST para Pistas
typedef struct PistaNode {
    char texto[Maxstring];
    struct PistaNode *esq;
    struct PistaNode *dir;
} PistaNode;

// Lista de pistas por suspeito
typedef struct PistaItem {
    char texto[Maxstring];
    struct PistaItem *next;
} PistaItem;

// Suspeito: nó na tabela hash (lista encadeada para colisões)
typedef struct Suspeito {
    char nome[Maxstring];
    PistaItem *pistas;
    int contador; // quantas pistas associadas (para decidir o mais provável)
    struct Suspeito *next; // para colisões na tabela
} Suspeito;

// ---------- Prototipos ----------
void LimparBuffer();
void LimparTela();

Sala* criarSala(const char* nome);
void conectarEsquerda(Sala* pai, Sala* filho);
void conectarDireita(Sala* pai, Sala* filho);
void explorarSalas(Sala* root);

PistaNode* inserirPistaBST(PistaNode* root, const char* texto, bool* inseriu);
void listarPistasEmOrdem(PistaNode* root);

void inicializarHash();
unsigned int hash_string(const char* s);
Suspeito* buscarSuspeito(const char* nome);
void inserirHash(const char* pista, const char* suspeitoNome);
void listarAssociacoes();
void mostrarSuspeitoMaisProvavel();

bool pistaExisteNoSuspeito(Suspeito* s, const char* pista);
void adicionarPistaAoSuspeito(Suspeito* s, const char* pista);

// ---------- Variaveis Globais ----------
PistaNode* pistasRoot = NULL;
Suspeito* tabelaHash[HASH_SIZE];

// Mapeamento sala -> pista e suspeito (simples): ao entrar numa sala específica, coleta pista
typedef struct MapEntry {
    const char* salaNome;
    const char* pista;
    const char* suspeito;
} MapEntry;

MapEntry mapaPistas[] = {
    {"Biblioteca", "Marca de lama no tapete", "Carlos"},
    {"Cozinha", "Panelas desalinhadas e cheiro forte", "Mariana"},
    {"Sotao", "Carta rasgada com iniciais 'R.'", "Rafael"},
    {"Hall de Entrada", "Pegada pequena perto da janela", "Ana"},
    {"Jardim", "Fita azul presa em um galho", "Mariana"},
    {"Escritorio", "Caneta com tinta vermelha", "Carlos"},
    {NULL, NULL, NULL}
};

// ---------- Implementacoes ----------

void LimparBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void LimparTela()
{
    for (int i = 0; i < 30; i++) printf("\n");
}

Sala* criarSala(const char* nome) {
    Sala* novo = (Sala*) malloc(sizeof(Sala));
    if (!novo) {
        fprintf(stderr, "Erro de alocacao\n");
        exit(1);
    }
    strncpy(novo->nome, nome, Maxstring-1);
    novo->nome[Maxstring-1] = '\0';
    novo->esquerda = novo->direita = NULL;
    novo->pai = NULL;
    return novo;
}

void conectarEsquerda(Sala* pai, Sala* filho) {
    if (!pai || !filho) return;
    pai->esquerda = filho;
    filho->pai = pai;
}

void conectarDireita(Sala* pai, Sala* filho) {
    if (!pai || !filho) return;
    pai->direita = filho;
    filho->pai = pai;
}

// ---------- BST de Pistas ----------
PistaNode* criarPistaNode(const char* texto) {
    PistaNode* n = (PistaNode*) malloc(sizeof(PistaNode));
    if (!n) { fprintf(stderr, "Erro de alocacao\n"); exit(1); }
    strncpy(n->texto, texto, Maxstring-1);
    n->texto[Maxstring-1] = '\0';
    n->esq = n->dir = NULL;
    return n;
}

// retorna nova raiz; seta *inseriu = true se inseriu (false se já existia)
PistaNode* inserirPistaBST(PistaNode* root, const char* texto, bool* inseriu) {
    if (!root) {
        *inseriu = true;
        return criarPistaNode(texto);
    }
    int cmp = strcmp(texto, root->texto);
    if (cmp == 0) {
        *inseriu = false;
        return root; // já existe
    } else if (cmp < 0) {
        root->esq = inserirPistaBST(root->esq, texto, inseriu);
    } else {
        root->dir = inserirPistaBST(root->dir, texto, inseriu);
    }
    return root;
}

void listarPistasEmOrdem(PistaNode* root) {
    if (!root) return;
    listarPistasEmOrdem(root->esq);
    printf("- %s\n", root->texto);
    listarPistasEmOrdem(root->dir);
}

// ---------- Tabela Hash e Suspeitos ----------
void inicializarHash() {
    for (int i = 0; i < HASH_SIZE; i++) tabelaHash[i] = NULL;
}

unsigned int hash_string(const char* s) {
    unsigned int sum = 0;
    for (const char* p = s; *p; ++p) sum = sum * 31 + (unsigned char)(*p);
    return sum % HASH_SIZE;
}

Suspeito* buscarSuspeito(const char* nome) {
    unsigned int h = hash_string(nome);
    Suspeito* cur = tabelaHash[h];
    while (cur) {
        if (strcmp(cur->nome, nome) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

bool pistaExisteNoSuspeito(Suspeito* s, const char* pista) {
    PistaItem* p = s->pistas;
    while (p) {
        if (strcmp(p->texto, pista) == 0) return true;
        p = p->next;
    }
    return false;
}

void adicionarPistaAoSuspeito(Suspeito* s, const char* pista) {
    if (!s) return;
    if (pistaExisteNoSuspeito(s, pista)) return;
    PistaItem* item = (PistaItem*) malloc(sizeof(PistaItem));
    strncpy(item->texto, pista, Maxstring-1);
    item->texto[Maxstring-1] = '\0';
    item->next = s->pistas;
    s->pistas = item;
    s->contador++;
}

void inserirHash(const char* pista, const char* suspeitoNome) {
    if (!suspeitoNome || strlen(suspeitoNome) == 0) return;
    unsigned int h = hash_string(suspeitoNome);
    Suspeito* existente = buscarSuspeito(suspeitoNome);
    if (existente) {
        // adiciona pista se ainda não existe
        adicionarPistaAoSuspeito(existente, pista);
        return;
    }
    // cria novo suspeito e insere no bucket
    Suspeito* s = (Suspeito*) malloc(sizeof(Suspeito));
    strncpy(s->nome, suspeitoNome, Maxstring-1);
    s->nome[Maxstring-1] = '\0';
    s->pistas = NULL;
    s->contador = 0;
    s->next = tabelaHash[h];
    tabelaHash[h] = s;
    // adiciona pista
    adicionarPistaAoSuspeito(s, pista);
}

void listarAssociacoes() {
    printf("Suspeitos e pistas associadas:\n");
    for (int i = 0; i < HASH_SIZE; i++) {
        Suspeito* cur = tabelaHash[i];
        while (cur) {
            printf("- %s (pistas: %d)\n", cur->nome, cur->contador);
            PistaItem* p = cur->pistas;
            while (p) {
                printf("    * %s\n", p->texto);
                p = p->next;
            }
            cur = cur->next;
        }
    }
}

void mostrarSuspeitoMaisProvavel() {
    Suspeito* melhor = NULL;
    for (int i = 0; i < HASH_SIZE; i++) {
        Suspeito* cur = tabelaHash[i];
        while (cur) {
            if (!melhor || cur->contador > melhor->contador) {
                melhor = cur;
            }
            cur = cur->next;
        }
    }
    if (!melhor) {
        printf("Nenhum suspeito registrado ainda.\n");
    } else {
        printf("Suspeito mais provável: %s (pistas: %d)\n", melhor->nome, melhor->contador);
        if (melhor->pistas) {
            printf("Pistas associadas:\n");
            PistaItem* p = melhor->pistas;
            while (p) {
                printf(" - %s\n", p->texto);
                p = p->next;
            }
        }
    }
}

// ---------- Auxiliares: buscar mapeamento sala -> pista/suspeito ----------
void processarEntradaSala(const char* nomeSala) {
    // percorre mapaPistas e, se achar uma entrada com mesmo nome, registra pista
    for (int i = 0; mapaPistas[i].salaNome != NULL; i++) {
        if (strcmp(nomeSala, mapaPistas[i].salaNome) == 0) {
            const char* pista = mapaPistas[i].pista;
            const char* suspeito = mapaPistas[i].suspeito;

            // inserir na BST (se ainda nao existente)
            bool inseriu = false;
            pistasRoot = inserirPistaBST(pistasRoot, pista, &inseriu);
            if (inseriu) {
                printf("[Nova pista coletada!]: %s\n", pista);
            } else {
                printf("[Pista já coletada anteriormente]: %s\n", pista);
            }

            // associar a suspeito na hash (se já coletada, a funcao evita duplicar)
            inserirHash(pista, suspeito);
            return;
        }
    }
    // se não houver pista para a sala:
    printf("[Nenhuma pista encontrada aqui]\n");
}

// ---------- Exploração interativa da mansão ----------
void explorarSalas(Sala* root) {
    Sala* atual = root;
    char opcao[16];

    while (true) {
        printf("Você está em: %s\n", atual->nome);
        // processa entrada da sala: coleta pista (se aplicável)
        processarEntradaSala(atual->nome);

        printf("Opções: (e) esquerda, (d) direita, (v) voltar/pai, (p) listar pistas, (s) sair exploração\n");
        printf("Escolha: ");
        if (!fgets(opcao, sizeof(opcao), stdin)) break;
        // remove newline
        opcao[strcspn(opcao, "\n")] = '\0';

        if (strcmp(opcao, "e") == 0) {
            if (atual->esquerda) {
                atual = atual->esquerda;
            } else {
                printf("Não há sala à esquerda.\n");
            }
        } else if (strcmp(opcao, "d") == 0) {
            if (atual->direita) {
                atual = atual->direita;
            } else {
                printf("Não há sala à direita.\n");
            }
        } else if (strcmp(opcao, "v") == 0) {
            if (atual->pai) {
                atual = atual->pai;
            } else {
                printf("Você já está na raiz, não há pai.\n");
            }
        } else if (strcmp(opcao, "p") == 0) {
            printf("Pistas coletadas (ordem alfabetica):\n");
            listarPistasEmOrdem(pistasRoot);
        } else if (strcmp(opcao, "s") == 0) {
            printf("Saindo da exploração...\n");
            break;
        } else {
            printf("Opcao invalida.\n");
        }
    }
}

// ---------- Função para construir mansão exemplo ----------
Sala* construirMansaoExemplo() {
    // cria salas
    Sala* hall = criarSala("Hall de Entrada");
    Sala* biblioteca = criarSala("Biblioteca");
    Sala* cozinha = criarSala("Cozinha");
    Sala* sotao = criarSala("Sotao");
    Sala* jardim = criarSala("Jardim");
    Sala* escritorio = criarSala("Escritorio");

    // montar conexões (exemplo)
    conectarEsquerda(hall, biblioteca);   // hall->esquerda = biblioteca
    conectarDireita(hall, cozinha);       // hall->direita = cozinha

    conectarEsquerda(biblioteca, sotao);  // biblioteca->esquerda = sotao
    conectarDireita(biblioteca, escritorio); // biblioteca->direita = escritorio

    conectarDireita(cozinha, jardim);     // cozinha->direita = jardim

    return hall; // raiz
}

// ---------- Main ----------
int main() {
    inicializarHash();

    Sala* mansao = construirMansaoExemplo();

    char opcao[16];
    while (true) {
        printf("\n=== Detective Quest (versão demo) ===\n");
        printf("1 - Explorar mansão\n");
        printf("2 - Listar pistas (ordem alfabetica)\n");
        printf("3 - Listar suspeitos e suas pistas\n");
        printf("4 - Mostrar suspeito mais provável\n");
        printf("5 - Sair\n");
        printf("Escolha: ");
        if (!fgets(opcao, sizeof(opcao), stdin)) break;
        opcao[strcspn(opcao, "\n")] = '\0';

        if (strcmp(opcao, "1") == 0) {
            explorarSalas(mansao);
        } else if (strcmp(opcao, "2") == 0) {
            printf("Pistas coletadas (ordem alfabetica):\n");
            listarPistasEmOrdem(pistasRoot);
        } else if (strcmp(opcao, "3") == 0) {
            listarAssociacoes();
        } else if (strcmp(opcao, "4") == 0) {
            mostrarSuspeitoMaisProvavel();
        } else if (strcmp(opcao, "5") == 0) {
            printf("Tchau!\n");
            break;
        } else {
            printf("Opcao invalida.\n");
        }
    }

    return 0;
}




