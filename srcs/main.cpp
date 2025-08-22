#include "statorGui.hpp"

int	main(int ac, char** av) {
	StatorGui       stator("./Parts.json", "./Recipes.json", "./UserRecipes.json");
	HephResult	    result = stator.create();

	HEPH_PRINT_RESULT(result);

	if (!result.valid()) {
		return (1);
	}

	stator.run();
	stator.destroy();
	return (0);
}
