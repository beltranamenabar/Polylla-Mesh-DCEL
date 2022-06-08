#include <iostream>
#include <string>

#include <polylla.hpp>
#include <triangulation.hpp>

int main(int argc, char **argv) {
    std::string output;

    if(argc == 5) {
        const auto node_file = std::string(argv[1]);
        const auto ele_file = std::string(argv[2]);
        const auto neigh_file = std::string(argv[3]);
        output = std::string(argv[4]);

        Triangulation tr(node_file, ele_file, neigh_file);
        Polylla mesh(tr);
        mesh.construct_polylla();

        mesh.print_OFF(output + ".off");
        std::cout << "output off in " << output << ".off" << std::endl;
        mesh.print_ale(output + ".ale");
        std::cout << "output ale in " << output << ".ale" << std::endl;

    } else if(argc == 3) {
        const auto off_file = std::string(argv[1]);
        output = std::string(argv[2]);

        Triangulation tr(off_file);
        Polylla mesh(tr);
        mesh.construct_polylla();

        mesh.print_OFF(output + ".off");
        std::cout << "output off in " << output << ".off" << std::endl;
        mesh.print_ale(output + ".ale");
        std::cout << "output ale in " << output << ".ale" << std::endl;

    } else {
        std::cout << "Usages:\t" << argv[0] << " <off file> <output name>" << std::endl;
        std::cout << "       \t" << argv[0] << " <node_file> <ele_file> <neigh_file> <output name>" << std::endl;
        return 1;
    }

    return 0;
}
