# Names of each game's mod for the build scripts (dot-sourced by build.ps1,
# deploy.ps1 and make_release.ps1). The C++ side reads the same names from
# games\<game>\game.h: keep both in step.
$Games = [ordered]@{
    dao = @{
        Title     = 'Dragon Age: Origins'
        Mod       = 'DAOAccess'
        Agent     = 'daoaccess.gfx'
        Docs      = 'BioWare\Dragon Age'
        Processes = @('DAOrigins', 'DAOriginsLauncher', 'DAOriginsConfig')
    }
    da2 = @{
        Title     = 'Dragon Age II'
        Mod       = 'DA2Access'
        Agent     = 'da2access.gfx'
        Docs      = 'BioWare\Dragon Age 2'
        Processes = @('DragonAge2', 'DragonAge2Launcher', 'DragonAge2Config')
    }
}

function Select-Games([string]$Game) {
    if ($Game -eq 'all') { return @($Games.Keys) }
    if (-not $Games.Contains($Game)) { throw "Unknown game '$Game' (dao, da2 or all)" }
    return @($Game)
}
