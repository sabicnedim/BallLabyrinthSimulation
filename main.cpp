#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <cmath>

// Funkcija koja pohranjuje koordinate kuglice u vektor tipa cv::Point
std::vector<cv::Point> uzmi_tacke(int redovi, int kolone, cv::Mat maska) {
    std::vector<cv::Point> tacke_na_kuglici;
    for (int i = 0; i < redovi; i++) {
        for (int j = 0; j < kolone; j++) {
            if (maska.at<uchar>(i, j) != 0) {
                tacke_na_kuglici.push_back(cv::Point(i, j));
            }
        }
    }
    return tacke_na_kuglici;
}

bool provjeriPreklapanjePrepreke(int y, int x, cv::Mat& slika_pom) {
    // Logika za provjeru da li datim koordinatama postoji prepreka
    return slika_pom.at<cv::Vec3b>(y, x) == cv::Vec3b(0, 0, 0); // Pretpostavka: crna boja označava prepreku
}



// Funkcija kojom se odredjuju krajnje tacke
void odredi_karakteristicne_tacke(const std::vector<cv::Point>& tacke_na_kuglici, int& minX, int& maxX, int& minY, int& maxY) {
    for (const auto& tacka : tacke_na_kuglici) {
        if (tacka.x < minX) minX = tacka.x;
        if (tacka.x > maxX) maxX = tacka.x;
        if (tacka.y < minY) minY = tacka.y;
        if (tacka.y > maxY) maxY = tacka.y;
    }
}

// Funkcija koja u vektor pohranjuje ivice kruznice da bi se detektovale crne putanje

void odredi_ivice(std::vector<cv::Point>& ivice, int& poluprecnik, int& x_koo_centra, int& y_koo_centra) {
    ivice.clear();
    for (int i = -poluprecnik; i <= poluprecnik; i++) {
        for (int j = 0; j <= poluprecnik; j++) { // SAMO DONJI DEO KUGLICE
            int x = x_koo_centra + i;
            int y = y_koo_centra + j;
            if (i * i + j * j <= poluprecnik * poluprecnik) {
                ivice.push_back(cv::Point(x, y));
            }
        }
    }
}


void kreci_se_po_y(int& y_koo_centra, std::vector<cv::Point>& tacke_na_kuglici){
    y_koo_centra++;
    for (auto& tacka : tacke_na_kuglici) {
        tacka.y += 1;
    }
}

int nadji_najmanji_zuti_piksel(int redovi, int kolone, cv::Mat maska) {
    std::vector<cv::Point> zute_tacke(0);
    for (int i = 0; i < redovi; i++) {
        for (int j = 0; j < kolone; j++) {
            if (maska.at<uchar>(i, j) != 0) {
                zute_tacke.push_back(cv::Point(i, j));
            }
        }
    }
    int maxy = zute_tacke[0].y;
    for(auto tac: zute_tacke){
        if(tac.x > maxy) maxy = tac.x;
    }
    return maxy;
}

int main() {
    cv::Mat slika = cv::imread("C://Users/AMD/Desktop/slike_png/sl4.png");
    if (slika.empty()) {
        std::cerr << "Neuspješno učitavanje slike!" << std::endl;
        return -1;
    }

    int sirina = slika.cols;
    int visina = slika.rows;
    int fps = 10;
    cv::VideoWriter animacija("C://Users//AMD/Desktop/animacija.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(sirina, visina));

    cv::Mat hsv, maska1, maska2, maska, zuta_maska;
    cv::cvtColor(slika, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(0, 20, 20), cv::Scalar(10, 255, 255), maska1);
    cv::inRange(hsv, cv::Scalar(160, 20, 20), cv::Scalar(180, 255, 255), maska2);
    cv::bitwise_or(maska1, maska2, maska);
    cv::inRange(hsv, cv::Scalar(20, 100, 100), cv::Scalar(30, 255, 255), zuta_maska);

    int najmanji_zuti_piksel = nadji_najmanji_zuti_piksel(zuta_maska.rows, zuta_maska.cols, zuta_maska);

    std::vector<cv::Point> tacke_na_kuglici = uzmi_tacke(maska.rows, maska.cols, maska);

    int minX = tacke_na_kuglici[0].x, maxX = tacke_na_kuglici[0].x;
    int minY = tacke_na_kuglici[0].y, maxY = tacke_na_kuglici[0].y;

    odredi_karakteristicne_tacke(tacke_na_kuglici, minX, maxX, minY, maxY);

    int poluprecnik = (maxX - minX) / 2;
    int x_koo_centra = (minY + maxY) / 2;
    int y_koo_centra = (minX + maxX) / 2;

    cv::Mat slika_pom = hsv.clone();
    bool ispod_zute_linije = false;

    std::vector<cv::Point> ivice;

    bool crtanje = true;
    bool desno = false, lijevo = false;
    bool zaustavi = false;
    bool dole = false;
    cv::Vec3b piksel;

    int broj_iteracija_po_frejmu = 3;

    while (!ispod_zute_linije) {

        for (int i = 0; i < broj_iteracija_po_frejmu; ++i) {

            bool slobodno_ispod = true;


            for (const auto& tacka : ivice) {
                int x = tacka.x;
                int y = tacka.y + 1;

                if (x >= 0 && x < slika.cols && y < slika.rows) {
                    cv::Vec3b boja = slika.at<cv::Vec3b>(y - 1, x);
                    int siva = (boja[0] + boja[1] + boja[2]) / 3;
                    if (siva < 50) {
                        slobodno_ispod = false;
                        break;
                    }
                }
            }

            if (slobodno_ispod) {
                y_koo_centra++;
                for (auto& tacka : tacke_na_kuglici)
                    tacka.y += 1;
            } else {
                //KLIZANJE DESNO
                bool moze_klizati_desno = false;
                for (const auto& tacka : ivice) {
                    int x = tacka.x;
                    int y = tacka.y;

                    if (x + 1 < slika.cols && y + 1 < slika.rows) {
                        cv::Vec3b ispod = slika.at<cv::Vec3b>(y + 1, x);
                        int siva_ispod = (ispod[0] + ispod[1] + ispod[2]) / 3;

                        bool ima_dijagonalno_prazno = false;
                        for (int offset = 1; offset <= 2; ++offset) {
                            int yy = y + offset;
                            if (yy < slika.rows && x + 1 < slika.cols) {
                                cv::Vec3b dij = slika.at<cv::Vec3b>(yy, x + 1);
                                int siva = (dij[0] + dij[1] + dij[2]) / 3;
                                if (siva > 50) {
                                    ima_dijagonalno_prazno = true;
                                    break;
                                }
                            }
                        }

                        if (siva_ispod < 50 && ima_dijagonalno_prazno) {
                            moze_klizati_desno = true;
                            break;
                        }
                    }
                }

                bool klizao = false;
                if (moze_klizati_desno) {
                    x_koo_centra++;
                    for (auto& tacka : tacke_na_kuglici)
                        tacka.x += 1;
                    klizao = true;
                }

                //KLIZANJE LIJEVO
                if (!klizao) {
                    bool moze_klizati_lijevo = false;
                    for (const auto& tacka : ivice) {
                        int x = tacka.x;
                        int y = tacka.y;

                        if (x - 1 >= 0 && y + 1 < slika.rows) {
                            cv::Vec3b ispod = slika.at<cv::Vec3b>(y + 2, x);
                            int siva_ispod = (ispod[0] + ispod[1] + ispod[2]) / 3;

                            bool ima_dijagonalno_prazno = false;
                            for (int offset = 1; offset <= 3; ++offset) {
                                int yy = y + offset;
                                if (yy < slika.rows && x - 1 >= 0) {
                                    cv::Vec3b dij = slika.at<cv::Vec3b>(yy, x - 1);
                                    int siva = (dij[0] + dij[1] + dij[2]) / 3;
                                    if (siva > 50) {
                                        ima_dijagonalno_prazno = true;
                                        break;
                                    }
                                }
                            }

                            if (siva_ispod < 255 && ima_dijagonalno_prazno) {
                                moze_klizati_lijevo = true;
                                break;
                            }
                        }
                    }

                    if (moze_klizati_lijevo) {
                        x_koo_centra--;
                        for (auto& tacka : tacke_na_kuglici)
                            tacka.x -= 1;
                    }
                }
            }


            if (ispod_zute_linije)
                break;


            ivice.clear();
            odredi_ivice(ivice, poluprecnik, x_koo_centra, y_koo_centra);

            if (y_koo_centra - poluprecnik > najmanji_zuti_piksel) {
                ispod_zute_linije = true;
            }


        }


        // CRTANJE
        slika_pom = hsv.clone();
        cv::cvtColor(slika_pom, slika_pom, cv::COLOR_HSV2BGR);
        cv::Point centar(x_koo_centra, y_koo_centra);
        for (const auto& iv : ivice) {
            if (iv.y >= 0 && iv.y < slika.rows && iv.x >= 0 && iv.x < slika.cols)
                slika_pom.at<cv::Vec3b>(iv.y, iv.x) = cv::Vec3b(0, 255, 255);
        }

        cv::cvtColor(slika_pom, slika_pom, cv::COLOR_BGR2HSV);
        slika_pom.setTo(cv::Scalar(0, 0, 255), maska);
        cv::cvtColor(slika_pom, slika_pom, cv::COLOR_HSV2BGR);
        cv::circle(slika_pom, centar, poluprecnik, cv::Scalar(0, 0, 255), -1, cv::LINE_AA);
        cv::imshow("Animacija", slika_pom);
        animacija.write(slika_pom);

        if (cv::waitKey(10) == 'q') break;
    }

    std::cout << najmanji_zuti_piksel;
    std::cout << y_koo_centra;

    animacija.release();
    return 0;
}
