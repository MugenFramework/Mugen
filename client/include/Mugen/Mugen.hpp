#ifndef MUGEN_MUGEN_HPP
#define MUGEN_MUGEN_HPP

#include <global.hpp>
#include <UserInterface/MugenUI.hpp>
#include <Mugen/DBManager/DBManager.hpp>

using namespace MugenNamespace;

class MugenSpace::Mugen {

    using toml_t = toml::basic_value<toml::discard_comments, unordered_map, vector>;;

public:
    toml_t Config;

    UserInterface::MugenUi MugenAppUI;
    DBManager* dbManager;
    QMainWindow* MugenMainWindow;
    bool ClientInitConnect = true;

    Mugen( QMainWindow* );
    ~Mugen();

    void Init( int argc, char** argv );
    void Start();

    static void Exit();
};

#endif
