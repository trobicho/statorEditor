#pragma once
#include <algorithm>
#include <boost/json.hpp>
#include <functional>
#include <imgui.h>
#include <imgui_internal.h>
#include <memory>
#include <string>
#include <vector>
#include "ImNodeFlow.h"
#include "data.hpp"

enum StatorNodeType {
  SNT_NA,
  SNT_FACTORY_NODE,
  SNT_RECIPE_NODE,
  SNT_PART_NODE,
  SNT_IN_NODE,
  SNT_OUT_NODE,
  SNT_BALANCE_NODE,
};

StatorNodeType  sntFromString(std::string type);

namespace json = boost::json;

struct  StatorNode : ImFlow::BaseNode {
  StatorNode(){
    nodeStyle = ImFlow::NodeStyle::cyan();
    pinStyle = ImFlow::PinStyle::brown();
    pinStyle->extra.padding=ImVec2(20.f,20.f);
    //pinStyle->extra.socket_padding=20.f;
    //pinStyle->extra.bg_color=IM_COL32(255,255,255,100);
    pinStyle->extra.link_thickness=10.f;
    setStyle(nodeStyle);

    pinRenderer=[](ImFlow::Pin* p){
      p->drawSocket();
      p->drawDecoration();
    };
  }

  virtual void            drawPopUp() {;}
  virtual StatorNodeType  statorNodeType() = 0;
  virtual json::value     toJson() = 0;
  virtual void            fromJson(json::value value) {;}
  virtual void            balance(std::vector<double> contr){;}

  void  recieveConstraint(double c) {
    int nbC = constraints.size();
    int nbOut = getOuts().size();

    if(nbC+1 == nbOut){
      constraints.push_back(c);
      printConstr();
      balance(constraints);
      isBalanced=true;
      constraints.clear();
    }
    else{
      constraints.push_back(c);
      isBalanced=false;
    }

  }

  void printConstr(){
    long id = getUID();
    std::cout << id << std::endl << "constr" << std::endl;
    for(auto d: constraints){
      std::cout << d << " , ";
    }
    std::cout << std::endl;
  }
  
  double  calcJ(std::vector<double> constr) {
    double v = 0.f;
    for(auto c: constr)
      v += c;
    balancedValue = v;
    return v;
  }

  void propagate(double c) {
    for(auto in: getIns()){
      if(!in->isConnected()){
        continue;
      }
      ImFlow::Pin* a=in->getLink().lock()->left();
      StatorNode* n=static_cast<StatorNode*>(a->getParent());
      n->recieveConstraint(c);
    }
  }

  std::shared_ptr<ImFlow::NodeStyle> nodeStyle;
  std::shared_ptr<ImFlow::PinStyle>  pinStyle;
  std::function<void(ImFlow::Pin* p)> pinRenderer;
  std::vector<double> constraints;
  bool isBalanced = false;
  double balancedValue = 0;
};

struct  InputNode: public StatorNode {
  InputNode(double v = 0.0): value(v) {
    setTitle("Input");
    addOUT<double>("out")->behaviour([this](){return (value);});
  }

  void  balance(std::vector<double> constr) override {
    calcJ(constr);
    propagate(balancedValue);
  }

  void  draw() override {
    ImGui::SetNextItemWidth(100.f);
    if(isBalanced)
      value=balancedValue;
    ImGui::InputDouble("##Val", &value);
  }

  StatorNodeType  statorNodeType() override {
    return (SNT_IN_NODE);
  };

  json::value     toJson() override {
    json::object value = {
      {"type", "SNT_IN_NODE"},
      {"value", value},
    };
    return (value);
  }
  void            fromJson(json::value value) override {
    this->value = value.as_object()["value"].as_double();
  }

  double  value = 0.0;
};

struct  OutputNode: public StatorNode {
  OutputNode() {
    setTitle("Output");
    addIN<double>("in", 0, ImFlow::ConnectionFilter::SameType());
  }

  void  draw() override {
    double r = this->getInVal<double>("in");
    ImGui::SetNextItemWidth(100.f);
    ImGui::Text("%f", r);
  }

  StatorNodeType  statorNodeType() override {
    return (SNT_OUT_NODE);
  };

  json::value     toJson() override {
    json::object value = {
      {"type", "SNT_OUT_NODE"},
    };
    return (value);
  }
};

struct  BalanceNode: public StatorNode {
  BalanceNode(){
    setTitle("Balance");
    addIO(0.0);
  }

  void  addIO(double c) {
    ioCount++;
    double count=ioCount;
    constraints.push_back(c);
    std::string cnt=std::to_string(ioCount);
    addIN<double>("in"+cnt, 0, ImFlow::ConnectionFilter::SameType(), pinStyle);
    addOUT<double>("out"+cnt, pinStyle)->behaviour([this, count](){
      return constraints[count-1];
    });
  }

  void  removeIO() {
    std::string cnt=std::to_string(ioCount);
    dropOUT("out"+cnt);
    dropIN("in"+cnt);
    constraints.pop_back();
    ioCount--;
  }

  void  draw() override {
    int i=1;
    for(auto& c: constraints){
      ImGui::SetNextItemWidth(100.f);
      ImGui::PushID(i);
      ImGui::InputDouble("##out", &c);
      ImGui::PopID();
      i++;
    }
    if(ImGui::Button("Balance")){
      std::cout << "balance" << std::endl;
      balance();
    }

    if(!(constraints == constraintsLast)){
      std::cout << "balance" << std::endl;
      balance();
    }
    constraintsLast=constraints;
  }

  void  balance() {
    if(!ioCount)
      return;
    int i=0;
    for(auto in: getIns()){
      if(!in->isConnected()){
        i++;
        continue;
      }
      ImFlow::Pin* a=in->getLink().lock()->left();
      StatorNode* n=static_cast<StatorNode*>(a->getParent());
      n->recieveConstraint(constraints[i]);
      i++;
    }
  }

  void  drawPopUp() override {
    ImGui::Separator();
    if (ImGui::Button("addIO"))
      addIO(1.0);
    if (ImGui::Button("removeIO")) {
      if (ioCount > 1) {
        removeIO();
      }
    }
  }

  StatorNodeType  statorNodeType() override {
    return (SNT_BALANCE_NODE);
  };

  json::value     toJson() override {
    json::object value = {
      {"type", "SNT_BALANCE_NODE"},
    };
    return (value);
  }

  int ioCount = 0;
  std::vector<double> constraints;
  std::vector<double> constraintsLast;
};

struct  PartNode: public StatorNode {
  PartNode(Part& a_part): part(a_part) {
    setTitle(part.name.c_str());
    setStyle(ImFlow::NodeStyle::red());

    addInPin();
    addOutPin();
  }
  ~PartNode() {
    for (auto& out: outRatios) {
      delete(out);
    }
  }

  struct  OutRatioPin {
    OutRatioPin(double r): ratio(r) {};

    double  calcQuantity(double quantity) {
      return (quantity * ratio);
    }

    double    ratio;
  };

  void  balance(std::vector<double> constr) override {
    calcJ(constr);
    int i=0;
    for(auto r: outRatios){
      r->ratio=constr[i] / balancedValue;
      i++;
    }
    propagate(balancedValue);
  }

  void  addInPin() {
    inCount += 1;
    std::string name = "in" + std::to_string(inCount);
    addIN<double>(name, 0, ImFlow::ConnectionFilter::SameType());
  }

  void  addOutPin(double r = 1.0) {
    OutRatioPin*  pin = new OutRatioPin(r);
    outRatios.push_back(pin);
    std::string name = "out" + std::to_string(outRatios.size());
    addOUT<double>(name)->behaviour([this, pin](){
      double quantity = 0.0;
      for (int i = 0; i < inCount; i++) {
        std::string name = "in" + std::to_string(i + 1);
        quantity += getInVal<double>(name);
      }
      return (pin->calcQuantity(quantity));
    });
  }
  void  removeIn() {
    if (inCount > 0) {
      std::string name = "in" + std::to_string(inCount);
      dropIN(name);
      inCount -= 1;
    }
  }
  void  removeOut() {
    if (outRatios.size() > 0) {
      std::string name = "out" + std::to_string(outRatios.size());
      dropOUT(name);
      delete(outRatios.back());
      outRatios.resize(outRatios.size() - 1);
    }
  }
  void  reset() {
    while (inCount > 0)
      removeIn();
    while (outRatios.size() > 0)
      removeOut();
  }

  void  draw() override {
    int i = 0;
    ImGui::Text("%f", balancedValue);
    for (auto& out: outRatios) {
      ImGui::SetNextItemWidth(100.f);
      ImGui::PushID(i);
      ImGui::InputDouble("##out", &out->ratio);
      ImGui::PopID();
      i++;
    }
  }

  void  drawPopUp() override {
    ImGui::Separator();
    if (ImGui::Button("addIn"))
      addInPin();
    ImGui::SameLine();
    if (ImGui::Button("addOut"))
      addOutPin();
    if (ImGui::Button("removeIn")) {
      if (inCount > 1) {
        removeIn();
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("removeOut"))
      removeOut();
  }

  StatorNodeType  statorNodeType() override {
    return (SNT_PART_NODE);
  };

  json::value     toJson() override {
    json::array outs;
    for (auto& out: outRatios) {
      outs.push_back(out->ratio);
    }
    json::object value = {
      {"type", "SNT_PART_NODE"},
      {"name", part.name},
      {"inCount", inCount},
      {"outs", outs},
    };
    return (value);
  }

  void            fromJson(json::value value) override {
    reset();
    int inN = value.as_object()["inCount"].as_int64();
    while (inCount < inN) {
      addInPin();
    }
    json::array outs = value.as_object()["outs"].as_array();
    for (auto& out: outs) {
      addOutPin(out.as_double());
    }
  }

  int                       inCount = 0;
  Part&                     part;
  std::vector<OutRatioPin*> outRatios;
};


struct  RecipeNode: public StatorNode {
  RecipeNode(Recipe& a_recipe): recipe(a_recipe) {
    setTitle("Recipe");
    setStyle(ImFlow::NodeStyle::green());
    for (auto& in: recipe.inputs) {
      addIN<double>(in.name, 0, ImFlow::ConnectionFilter::SameType());
    }
    for (auto& out: recipe.outputs) {
      addOUT<double>(out.name)->behaviour([this, out](){
        double ratioMin = calcRatio();
        return (ratioMin * out.quantity);
      });
    }
  }

  void balance(std::vector<double> constr) override{
    std::cout << "recipeBalance" << std::endl;
    //for(auto c: constr){
      int i=0;
      for(auto in: getIns()){
        if(!in->isConnected())
          continue;
        ImFlow::Pin* a=in->getLink().lock()->left();
        StatorNode* n=static_cast<StatorNode*>(a->getParent());
        //Need For multiple output recipes ----------------------------------------------------------------------!!!!!!!!!!!!
        n->recieveConstraint(constr[0] / getTransform(recipe.inputs[i], recipe.outputs[0]));
        i++;
      }
    //}
  }

  double  getTransform(PartWithQuantity& in, PartWithQuantity& out) {
    return out.quantity / in.quantity;
  }

  inline double  calcRatio(PartWithQuantity& in) {return (getInVal<double>(in.name) / in.quantity);}
  double  calcRatio() {
    double ratioMin = calcRatio(recipe.inputs[0]);
    for (int i = 1; i < recipe.inputs.size(); i++) {
      double r = calcRatio(recipe.inputs[i]);
      if (ratioMin > r)
        ratioMin = r;
    }
    return (ratioMin);
  }

  void  drawPopUp() override {
    ImGui::Separator();
    recipe.drawPopUp();
    double  ratioMin = calcRatio();
    ImGui::Separator();
    ImGui::Text("Surplus:");
    for (auto& in: recipe.inputs) {
      double inVal = this->getInVal<double>(in.name);
      double r = inVal / in.quantity;
      if (r > ratioMin)
        ImGui::Text("%s: %lf", in.name.c_str(), inVal * (r - ratioMin));
    }
  }

  StatorNodeType  statorNodeType() override {
    return (SNT_RECIPE_NODE);
  };

  json::value     toJson() override {
    json::object value = {
      {"type", "SNT_RECIPE_NODE"},
      {"recipeId", recipe.id},
    };
    return (value);
  }

  Recipe&   recipe;
};
