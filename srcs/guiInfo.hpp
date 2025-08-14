#pragma once

#include <imgui.h>

struct	GuiWindowInfo {
	GuiWindowInfo() {}
	GuiWindowInfo(ImVec2 a_pos, ImVec2 a_size): pos(a_pos), size(a_size) {}

	void		getInfo() {
		pos = ImGui::GetWindowPos();
		size = ImGui::GetWindowSize();
	}
	void		setNext() {
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);
	}
	void		set() {
		ImGui::SetWindowPos(pos);
		ImGui::SetWindowSize(size);
	}

	ImVec2	pos = ImVec2(0, 0);
	ImVec2	size = ImVec2(0, 0);
};
