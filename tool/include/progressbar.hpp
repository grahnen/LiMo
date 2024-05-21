#ifndef PROGRESSBAR_H_
#define PROGRESSBAR_H_



void print_bar(std::ostream &os, float progress) {
    const int barWidth = 70;

    os << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) os << "=";
        else if (i == pos) os << ">";
        else os << " ";
    }
    os << "] " << int(progress * 100.0) << " %\r";
    os.flush();
}

#endif // PROGRESSBAR_H_
