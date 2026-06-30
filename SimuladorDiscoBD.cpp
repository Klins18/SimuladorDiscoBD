#include <SFML/Graphics.hpp>
#include <iostream>
#include <imgui.h>
#include <imgui-SFML.h>
#include "Disco_funciones.h"
using namespace std;

int main() {
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "Simulador de Disco - Prueba");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);
    ImGui::GetIO().Fonts->AddFontDefault();

    sf::Clock clock;

    int cantidad_platos = 2;
	int cantidad_pistas = 3;
	int cantidad_sectores = 4;
	int capacidad_bytes = 10;

	char buffer[256] = "";
	bool disco_inicializado = true;

	Disco disco = inicializar_disco(cantidad_platos, cantidad_pistas, cantidad_sectores, capacidad_bytes);
	/*
    escribir_registro(disco,"Lima|20|Ingenieria de Sistemas|Peru");
    
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 3; k++) {
                for (int s = 0; s < 4; s++) {
                    Sector& sec = disco.platos[i].superficies[j].pistas[k].sectores[s];
                    if (!sec.libre) {
                        cout << "Plato:" << i << " Sup:" << j
                            << " Pista:" << k << " Sector:" << s
                            << " | Datos: ";
                        for (int b = 0; b < sec.capacidad_bytes; b++)
                            cout << sec.datos[b];
                        cout << " | Continuacion: " << sec.es_continuacion << endl;
                    }
                }
            }
        }
    }
    */
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        ImGui::SFML::Update(window, clock.restart());

        ImGui::Begin("Configuracion del Disco");
        ImGui::InputInt("Cantidad de Platos", &cantidad_platos);
        ImGui::InputInt("Cantidad de Pistas", &cantidad_pistas);
        ImGui::InputInt("Cantidad de Sectores", &cantidad_sectores);
        ImGui::InputInt("Capacidad del Sector", &capacidad_bytes);
        if (ImGui::Button("Inicializar Disco", ImVec2(150, 30))) {
			liberar_disco(disco);
			disco_inicializado = true;
            disco = inicializar_disco(cantidad_platos, cantidad_pistas, cantidad_sectores, capacidad_bytes);
        }
        ImGui::End();

        ImGui::Begin("Ingresar Registro");
		ImGui::InputText("Dato", buffer, sizeof(buffer));
        if (ImGui::Button("Escribir", ImVec2(150, 30))) {
			if (disco_inicializado) {
				escribir_registro(disco, buffer);
            }
        }
        ImGui::End();

        ImGui::Begin("Estado del Disco");
        if (ImGui::BeginTable("sectores", 5)) {
            ImGui::TableSetupColumn("Plato");
            ImGui::TableSetupColumn("Sup");
            ImGui::TableSetupColumn("Pista");
            ImGui::TableSetupColumn("Sector");
            ImGui::TableSetupColumn("Datos", ImGuiTableColumnFlags_WidthFixed, 200.0f);
            ImGui::TableHeadersRow();

            for (int i = 0; i < disco.cantidad_platos; i++) {
                for (int j = 0; j < 2; j++) {
                    for (int k = 0; k < disco.platos[i].superficies[j].cantidad_pistas; k++) {
                        for (int s = 0; s < disco.platos[i].superficies[j].pistas[k].cantidad_sectores; s++) {
                            Sector& sec = disco.platos[i].superficies[j].pistas[k].sectores[s];
                            if (!sec.libre) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);
                                ImGui::TableSetColumnIndex(1); ImGui::Text("%d", j);
                                ImGui::TableSetColumnIndex(2); ImGui::Text("%d", k);
                                ImGui::TableSetColumnIndex(3); ImGui::Text("%d", s);
                                ImGui::TableSetColumnIndex(4); ImGui::Text("%.*s", sec.capacidad_bytes, sec.datos);
                            }
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();

        window.clear(sf::Color(30, 30, 30));
        ImGui::SFML::Render(window);
        window.display();
    }

	liberar_disco(disco);
    ImGui::SFML::Shutdown();
    return 0;
}