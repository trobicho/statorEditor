#pragma once
#include <boost/json.hpp>
#include <imgui.h>
#include <string>

namespace json = boost::json;

struct  PartWithQuantity {
  PartWithQuantity() {};
  PartWithQuantity(json::object part) {
    name = part["Part"].as_string();
    if (part["Quantity"].is_int64())
      quantity = (double)part["Quantity"].as_int64();
    else
      quantity = (double)part["Quantity"].as_double();
  }

  std::string   name;
  double        quantity;
};

struct  Recipe ;

struct  Part {
  Part(json::object partJson) {
    name = partJson["name"].as_string();
    imgPath = partJson["img"].as_string();
  }

  void  addRecipe(Recipe& recipe) {
    recipes.push_back(&recipe);
  }

  std::string           name;
  std::string           imgPath;
  std::vector<Recipe*>  recipes;
};

struct  Recipe {
  Recipe(json::object recipeJson) {
    id = recipeJson["RecipeId"].as_int64();
    for (auto& input: recipeJson["Input"].as_array()) {
      inputs.push_back(PartWithQuantity(input.as_object()));
    }
    for (auto& output: recipeJson["Output"].as_array()) {
      outputs.push_back(PartWithQuantity(output.as_object()));
    }
  }
  
  void  drawPopUp() {
    ImGui::Text("IN:");
    for (auto& in: inputs) {
      ImGui::Text("%s, %lf", in.name.c_str(), in.quantity);
    }
    ImGui::Text("OUT:");
    for (auto& out: outputs) {
      ImGui::Text("%s, %lf", out.name.c_str(), out.quantity);
    }
  }

  int                             id;
  std::vector<PartWithQuantity>   inputs; 
  std::vector<PartWithQuantity>   outputs; 
};

static std::vector<Part>    partsGlobalArray;
static std::vector<Recipe>  recipesGlobalArray;
