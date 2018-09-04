# Table of Contents
- [Building the Digimesh wrapper](#digimesh-build)
  - [Qt creator IDE](#digimesh-qt-build)
  - [Command line](#digimesh-command-line-build)
- [Setting Environment Variables](#env-vars)
  - [Windows](#windows-env-vars)
  - [Linux](#linux-env-vars)

# <a name="digimesh-build"></a> Building the Digimesh Wrapper
Clone the repository
```
$ git clone https://github.com/heronsystems/MACEDigiWrapper
```

In the directory where the wrapper was cloned, create three new directories: `MACEDigiWrapper/lib`, `MACEDigiWrapper/bin`, and `MACEDigiWrapper/include`.

There are two options when building the Digimesh wrapper. Follow the instructions for your preferred method:

- [Qt Creator IDE build](#digimesh-qt-build)
- [Command line build](#digimesh-command-line-build)

## <a name="digimesh-qt-build"></a> Qt Creator IDE build
Using the Qt Creator IDE, open the `MACEDigiWrapper.pro` file, making sure to configure the project using a `MinGW` enabled kit. Go to the `Projects` tab and select `Build`. 

![Qt_ProjectsTab](https://github.com/heronsystems/MACE/blob/master/docs/images/Qt_ProjectsTab.png)

Add a build step using the dropdown and select `Make`. 

![Qt_MakeDropdown](https://github.com/heronsystems/MACE/blob/master/docs/images/Qt_MakeDropdown.png)

In the `Make arguments:` box, add `install`. Generated headers and libraries should install to the `/include` and `/lib` directories off the root of the project made prior to building.

![Qt_MakeInstall](https://github.com/heronsystems/MACE/blob/master/docs/images/Qt_MakeInstall.png)

## <a name="digimesh-command-line-build"></a> Command line build
Navigate to `/MACEDigiWrapper` and use `qmake` to generate Makefiles. Then `make` the libraries and install header files with the following commands:
```
$ cd /_code/MACEDigiWrapper
$ qmake
$ make
$ make install
```

# <a name="env-vars"></a> Digimesh Environment Variables
To build MACE later, you will need to set environment variables for the MACEDigiWrapper. The steps to do so are different between Windows and Linux.

## <a name="env-vars-windows"></a> Windows
Open the Windows Control Panel and search for "Environment Variables." Select `Edit the system environment variables` and select "Environment Variables..." You should see the "Environment Variables" window appear:

![EnvironmentVariables](https://github.com/heronsystems/MACE/blob/master/docs/images/EnvironmentVariables.png)

Select "New..." and add an environment variable with the name `MACE_DIGIMESH_WRAPPER` and the value set to the root directory such that `%MACE_DIGIMESH_WRAPPER%/include` and `%MACE_DIGIMESH_WRAPPER%/lib` resolves to the appropriate directories. If the library was cloned into `C:/Code`, the variable value would be `C:/Code/MACEDigiWrapper`. There is no need to add anything to the `PATH` environment variable at this point. 


## <a name="env-vars-linux"></a> Linux
Open the `~/.bashrc` file with your preferred text editor and add the following lines to the end of the file:

```
export MACE_DIGIMESH_WRAPPER="/_code/MACEDigiWrapper"
```

Make sure to run `source ~/.bashrc` to have the changes take effect. This edit to the `.bashrc` file will have the `MACE_DIGIMESH_WRAPPER` set in every terminal. The above path (i.e. `/_code/MACEDigiWrapper`) is an example, however this path should reflect wherever the root directory of the MACEDigiWrapper repository is. The `MACE_DIGIMESH_WRAPPER` environment variable should be set such that `%MACE_DIGIMESH_WRAPPER%/include` and `%MACE_DIGIMESH_WRAPPER%/lib` resolves to the appropriate directories. 
