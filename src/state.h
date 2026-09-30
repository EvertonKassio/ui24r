/*
 * state.h - Estado (local, em memoria) de mute de cada canal.
 *
 * POR ENQUANTO (sem acesso a mesa) tudo isso e so local: tocar em um
 * botao apenas muda a cor na tela. Quando a comunicacao com a Ui24R
 * for implementada, o lugar certo para "plugar" isso e a funcao
 * enviar_mute_para_mesa() no fim deste arquivo - ela ja e chamada
 * toda vez que o usuario muda o estado de um canal pela interface.
 */
#pragma once
#include <stdint.h>
#include "data.h"

/* true = canal ativado (som passa); false = mutado */
extern bool estado_canal[];

void estado_init();

bool canal_get_ativo(int indice_canal);
void canal_set_ativo(int indice_canal, bool ativo);
void canal_alternar(int indice_canal);

/* Um grupo aparece "ativado" (botao preenchido) se pelo menos um dos
 * canais dele estiver ativo. Tocar no bloco do grupo alterna todos os
 * canais do grupo juntos (liga todos de uma vez, ou muta todos). */
bool grupo_get_ativo(int grupo_id);
void grupo_alternar(int grupo_id);

/* Chamada sempre que o usuario muda o estado de um canal pela tela:
 * envia o comando de mute para a mesa (se estiver conectada; se nao,
 * so o estado local muda). "numero_canal" e o numero 1-22 de data.h. */
void enviar_mute_para_mesa(int numero_canal, bool ativo);

/* Estado vindo DA MESA (alguem mexeu no mute no app/mesa, ou o
 * estado inicial enviado ao conectar). Atualiza o estado local SEM
 * reenviar nada para a mesa e avisa a interface via callback. */
typedef void (*EstadoMudouCb)(int indice_canal);
void estado_set_callback(EstadoMudouCb cb);
void canal_set_ativo_remoto(int indice_canal, bool ativo);
