#include "ArborBp.hpp"

#include <iostream>
#include <stdexcept>

ArbolBPlus::Nodo::Nodo(bool hoja, std::size_t orden)
	: esHoja(hoja), cantidadClaves(0), claves(new std::string[orden]),
	  envios(hoja ? new Envio*[orden] : nullptr),
	  hijos(hoja ? nullptr : new Nodo*[orden + 1]), siguiente(nullptr)
{
	if (!hoja) {
		for (std::size_t i = 0; i < orden + 1; ++i) {
			hijos[i] = nullptr;
		}
	}
}

ArbolBPlus::Nodo::~Nodo()
{
	delete[] claves;
	delete[] envios;
	delete[] hijos;
}

ArbolBPlus::Division::Division()
	: ocurrio(false), claveSeparadora(), hijoDerecho(nullptr)
{
}

ArbolBPlus::ArbolBPlus(std::size_t ordenArbol)
	: orden(ordenArbol), cantidadEnvios(0), raiz(nullptr)
{
	if (orden < 3) {
		throw std::invalid_argument("El orden del arbol B+ debe ser al menos 3");
	}
	raiz = new Nodo(true, orden);
}

ArbolBPlus::~ArbolBPlus()
{
	liberarNodo(raiz);
	raiz = nullptr;
}

bool ArbolBPlus::insertar(const std::string& clave, Envio* envio)
{
	if (envio == nullptr || buscar(clave) != nullptr) {
		return false;
	}

	Division division = insertarEnNodo(raiz, clave, envio);
	if (division.ocurrio) {
		Nodo* nuevaRaiz = new Nodo(false, orden);
		nuevaRaiz->claves[0] = division.claveSeparadora;
		nuevaRaiz->cantidadClaves = 1;
		nuevaRaiz->hijos[0] = raiz;
		nuevaRaiz->hijos[1] = division.hijoDerecho;
		raiz = nuevaRaiz;
	}

	++cantidadEnvios;
	return true;
}

Envio* ArbolBPlus::buscar(const std::string& clave) const
{
	Nodo* actual = raiz;
	while (actual != nullptr && !actual->esHoja) {
		std::size_t posicion = 0;
		while (posicion < actual->cantidadClaves && clave >= actual->claves[posicion]) {
			++posicion;
		}
		actual = actual->hijos[posicion];
	}

	if (actual == nullptr) {
		return nullptr;
	}

	for (std::size_t i = 0; i < actual->cantidadClaves; ++i) {
		if (actual->claves[i] == clave) {
			return actual->envios[i];
		}
	}
	return nullptr;
}

void ArbolBPlus::mostrarEnOrden() const
{
	Nodo* hoja = hojaInicial();
	while (hoja != nullptr) {
		for (std::size_t i = 0; i < hoja->cantidadClaves; ++i) {
			std::cout << hoja->claves[i] << " | ";
			if (hoja->envios[i]) {
				hoja->envios[i]->mostrar();
			} else {
				std::cout << "(null envio)";
			}
		}
		hoja = hoja->siguiente;
	}
}

std::size_t ArbolBPlus::cantidad() const
{
	return cantidadEnvios;
}

bool ArbolBPlus::estaVacio() const
{
	return cantidadEnvios == 0;
}

ArbolBPlus::Division ArbolBPlus::insertarEnNodo(
	Nodo* nodo, const std::string& clave, Envio* envio)
{
	if (nodo->esHoja) {
		return insertarEnHoja(nodo, clave, envio);
	}
	return insertarEnInterno(nodo, clave, envio);
}

ArbolBPlus::Division ArbolBPlus::insertarEnHoja(
	Nodo* hoja, const std::string& clave, Envio* envio)
{
	std::size_t posicion = 0;
	while (posicion < hoja->cantidadClaves && hoja->claves[posicion] < clave) {
		++posicion;
	}

	for (std::size_t i = hoja->cantidadClaves; i > posicion; --i) {
		hoja->claves[i] = hoja->claves[i - 1];
		hoja->envios[i] = hoja->envios[i - 1];
	}
	hoja->claves[posicion] = clave;
	hoja->envios[posicion] = envio;
	++hoja->cantidadClaves;

	if (hoja->cantidadClaves < orden) {
		return Division();
	}

	Nodo* nuevaHoja = new Nodo(true, orden);
	const std::size_t puntoMedio = hoja->cantidadClaves / 2;
	nuevaHoja->cantidadClaves = hoja->cantidadClaves - puntoMedio;

	for (std::size_t i = 0; i < nuevaHoja->cantidadClaves; ++i) {
		nuevaHoja->claves[i] = hoja->claves[puntoMedio + i];
		nuevaHoja->envios[i] = hoja->envios[puntoMedio + i];
	}
	hoja->cantidadClaves = puntoMedio;
	nuevaHoja->siguiente = hoja->siguiente;
	hoja->siguiente = nuevaHoja;

	Division division;
	division.ocurrio = true;
	division.claveSeparadora = nuevaHoja->claves[0];
	division.hijoDerecho = nuevaHoja;
	return division;
}

ArbolBPlus::Division ArbolBPlus::insertarEnInterno(
	Nodo* interno, const std::string& clave, Envio* envio)
{
	std::size_t posicion = 0;
	while (posicion < interno->cantidadClaves && clave >= interno->claves[posicion]) {
		++posicion;
	}

	Division division = insertarEnNodo(interno->hijos[posicion], clave, envio);
	if (!division.ocurrio) {
		return Division();
	}

	for (std::size_t i = interno->cantidadClaves; i > posicion; --i) {
		interno->claves[i] = interno->claves[i - 1];
		interno->hijos[i + 1] = interno->hijos[i];
	}
	interno->claves[posicion] = division.claveSeparadora;
	interno->hijos[posicion + 1] = division.hijoDerecho;
	++interno->cantidadClaves;

	if (interno->cantidadClaves < orden) {
		return Division();
	}

	Nodo* nuevoInterno = new Nodo(false, orden);
	const std::size_t puntoMedio = interno->cantidadClaves / 2;
	const std::string separadora = interno->claves[puntoMedio];
	nuevoInterno->cantidadClaves = interno->cantidadClaves - puntoMedio - 1;

	for (std::size_t i = 0; i < nuevoInterno->cantidadClaves; ++i) {
		nuevoInterno->claves[i] = interno->claves[puntoMedio + 1 + i];
	}
	for (std::size_t i = 0; i <= nuevoInterno->cantidadClaves; ++i) {
		nuevoInterno->hijos[i] = interno->hijos[puntoMedio + 1 + i];
	}
	interno->cantidadClaves = puntoMedio;

	Division resultado;
	resultado.ocurrio = true;
	resultado.claveSeparadora = separadora;
	resultado.hijoDerecho = nuevoInterno;
	return resultado;
}

ArbolBPlus::Nodo* ArbolBPlus::hojaInicial() const
{
	Nodo* actual = raiz;
	while (actual != nullptr && !actual->esHoja) {
		actual = actual->hijos[0];
	}
	return actual;
}

// Este metodo libera todo el arbol, cuidado
void ArbolBPlus::liberarNodo(Nodo* nodo)
{
	if (nodo == nullptr) {
		return;
	}
	if (!nodo->esHoja) {
		for (std::size_t i = 0; i <= nodo->cantidadClaves; ++i) {
			liberarNodo(nodo->hijos[i]);
		}
	}
	delete nodo;
}
