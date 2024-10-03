//
// Created by Igor Kluzhev on 25.09.2024.
//

#include "pnm.h"
#include <cmath>
#include <stdio.h>

PNMPicture::PNMPicture() = default;
PNMPicture::PNMPicture(string filename) {
    read(filename);
}

void PNMPicture::read(string fileName) {
    ifstream inputFile(fileName, ios::binary);
    if (!inputFile.is_open())
        throw FileIOException();
    read(inputFile);
    inputFile.close();
}

void PNMPicture::read(ifstream& inputFile) {
    char P;
    inputFile >> P;
    if (P != 'P')
        throw UnsupportedFormatException();

    inputFile >> format;
    short colorsCount;
    if (format == 5) {
        colorsCount = 1;
    } else if (format == 6) {
        colorsCount = 3;
    } else {
        throw UnsupportedFormatException();
    }
    inputFile >> width >> height;
    inputFile >> colors;

    inputFile.get();

    size_t dataSize = width * height * colorsCount;
    data.resize(dataSize);
    inputFile.read((char*) &data[0], dataSize);

    if (inputFile.fail())
        throw FileIOException();
}

void PNMPicture::write(const string& fileName) {
    ofstream outputFile(fileName, ios::binary);
    if (!outputFile.is_open())
        throw FileIOException();
    write(outputFile);
    outputFile.close();
}

void PNMPicture::write(ofstream& outputFile) {
    outputFile << "P" << format << '\n';
    outputFile << width << ' ' << height << '\n';
    outputFile << colors << '\n';
    short colorsCount;
    if (format == 5) {
        colorsCount = 1;
    } else if (format == 6) {
        colorsCount = 3;
    } else {
        throw UnsupportedFormatException();
    }
    outputFile.write((char*) &data[0], width * height * colorsCount);
}

void PNMPicture::printInfo() {
    cout << "Format = P" << format << endl;
    cout << "W x H = " << width << " x " << height << endl;
    cout << "Colors = " << colors << endl;
    cout << "Data size = " << data.size() << endl;

    size_t size = data.size();
    uchar min_v = 255;
    uchar max_v = 0;
    for (size_t i = 0; i < size; i++) {
        uchar v = data[i];
        min_v = min(min_v, v);
        max_v = max(max_v, v);
    }

    cout << "Min = " << int(min_v) << endl;
    cout << "Max = " << int(max_v) << endl << endl;
}

// 1) изначально при итерировании собираем кол-ва по каждому цвету
// дополнительно считаем сумму светлых и тёмных цветов
// 2) при итерировании по цветам суммируем кол-во для тёмных и светлых
// 3) при достижении нужного кол-ва - идём дальше
// и сохраняем первый попавшийся индекс с ненулевым значением как минимальный/максимальный цвет
// 4) вычисляем min/max
// 5) пробегаемся ещё раз и меняем значения
// не забыть посчитать коэффициент умножения заранее
// как оптимизация: для цветов можно сохранять умноженные значения в мапе (надо проверить будет ли профит)

// TODO: оптимизировать и сделать игнорирование для разных каналов раздельно
void PNMPicture::modify(float coeff, bool isDebug) {
    size_t size = data.size();

    int ignoreCount = size * coeff;
    if (isDebug) {
        cout << "Ignoring " << ignoreCount << " values at each side" << endl << endl;
    }

    vector<int> elements;
    elements.resize(256, 0);

    uchar min_v = 255;
    uchar max_v = 0;
    int darkFullCount = 0;
    int brightFullCount = 0;
    for (size_t i = 0; i < size; i++) {
        int v = data[i];
        elements[v] += 1;
        if (v < 128) {
            darkFullCount += 1;
        } else {
            brightFullCount += 1;
        }
    }

    int darkCount = 0;
    bool isDarkComplete = false;
    int brightCount = 0;
    bool isBrightComplete = false;

    for (size_t i = 0; i < 128; i++) {
        if (!isDarkComplete) {
            size_t darkIndex = i;
            int element = elements[darkIndex];
            if (darkCount < ignoreCount) {
                darkCount += element;
            }

            if (darkCount >= ignoreCount && element != 0) {
                isDarkComplete = true;
                min_v = darkIndex;
            }
        }

        if (!isBrightComplete) {
            size_t brightIndex = 255 - i;
            int element = elements[brightIndex];
            if (brightCount < ignoreCount) {
                brightCount += element;
            }

            if (brightCount >= ignoreCount && element != 0) {
                isBrightComplete = true;
                max_v = brightIndex;
            }
        }

        if (isDarkComplete && isBrightComplete) {
            break;
        }
    }

    if (isDebug) {
        cout << "Min = " << int(min_v) << endl;
        cout << "Max = " << int(max_v) << endl << endl;
    }

    if (min_v == 0 && max_v == 255) {
        return;
    }

    float scale = 255.0 / float(max_v - min_v);

    if (isDebug) {
        cout << "scale = " << scale << endl << endl;
    }

    for (size_t i = 0; i < size; i++) {
        // TODO: разобраться с типами и кастами
        uchar value = data[i];
        int scaledValue = int(scale * (int(value) - int(min_v)));
        uchar limitedValue = max(0, min(scaledValue, 255));
        data[i] = limitedValue;
    }
}