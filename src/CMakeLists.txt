file(GLOB SOURCES
    # *.hpp # Unnecessary, but convenient for IDE 
          # (adds headers to project)
    *.cpp
    )

# Set SOURCES for parent scope
set(SOURCES ${SOURCES} PARENT_SCOPE)