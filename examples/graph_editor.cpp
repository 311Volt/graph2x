
#include <graph2x.hpp>
#include <axxegro/axxegro.hpp>
#include <list>
#include <map>
#include <deque>


al::Vec3f ColorToVec(al::Color color) {
	return al::Vec3f{color.r, color.g, color.b};
}
al::Color VecToColor(al::Vec3f vec) {
	return al::RGB_f(vec.x, vec.y, vec.z);
}

float PointToLineSegmentDist(al::Vec2f p, al::Vec2f a, al::Vec2f b)
{
	float l2 = (b - a).sqLength();
	if (l2 == 0.0) 
		return (p - a).length();
	float t = std::max<float>(0, std::min<float>(1, (p - a).dot(b - a) / l2));
	al::Vec2f prj = a + (b - a)*t;
	return (p - prj).length();
}

template<typename T>
struct SmoothValue {
	T value {};
	T targetValue {};
	double halfLife = 1.0 / 12.0;

	void update(double deltaTime) {
		double w = std::pow(2.0, -deltaTime / halfLife);
		value = value*w + targetValue*(1.0-w);
	}


};

namespace msg {

	struct AddVertex {
		al::Vec2f worldPos;
	};

	struct StartDrawingVertexEdge {
		int beginVtxLabel;
	};

	struct FinishDrawingVertexEdge {
		int endVtxLabel;
	};

	struct DeleteVertex {
		int vtxLabel;
	};

	struct DeleteEdge {
		int edgeLabel;
	};

}

class World;


namespace glob {
	al::Font DefaultFont = al::Font::CreateBuiltinFont();
	al::UserEventSource WorldEventSource {};
	World* world = nullptr;

	inline struct {
		bool displayGraphGrid = false;
		bool displayNodeLabels = false;
	} cfg;
}

void DrawText(const std::string& text, al::Color color, al::Vec2i pos, int align = ALLEGRO_ALIGN_LEFT) {
	glob::DefaultFont.drawText(text, color, pos, align);
}

void DrawShadowedText(const std::string& text, al::Color color, al::Vec2i pos, int align = ALLEGRO_ALIGN_LEFT) {
	al::Color shadowColor = color;
	auto& [r, g, b, a] = shadowColor;
	r /= 4.0; g /= 4.0; b /= 4.0; a = 0.5;
	DrawText(text, shadowColor, pos + al::Vec2i{2, 2}, align);
	DrawText(text, color, pos, align);
}


struct Entity {
	virtual void tick(double deltaTime) = 0;
	virtual void render() = 0;
	virtual bool onEvent(const al::Event&) = 0;
	virtual int getZOrder() = 0;
	virtual ~Entity() = default;
};

struct World {
public:

	friend struct EntityHandle;

	template<typename T>
		requires std::is_base_of_v<Entity, T>
	struct EntityHandle {
		World* world;
		std::list<std::unique_ptr<Entity>>::iterator iter;

		EntityHandle(World* world, std::list<std::unique_ptr<Entity>>::iterator iter) 
			: world(world), iter(iter)
		{

		}

		EntityHandle(const EntityHandle&) = delete;
		EntityHandle& operator=(const EntityHandle&) = delete;
		EntityHandle(EntityHandle&& other): world(other.world), iter(other.iter) {
			other.valid = false;
		}
		EntityHandle& operator=(EntityHandle&& other) {
			(*this) = std::move(other);
		}

		[[nodiscard]] T& get() {
			return *static_cast<T*>(iter->get());
		}
		[[nodiscard]] const T& get() const {
			return *static_cast<const T*>(iter->get());
		}

		~EntityHandle() {
			if(valid) {
				world->entities.erase(iter);
			}
		}
	private:
		bool valid = true;
	};

	template<typename T, typename... Ts>
		requires std::is_base_of_v<Entity, T>
	EntityHandle<T> createEntity(Ts&&... args) {
		return EntityHandle<T> {
			this, 
			entities.emplace(entities.end(), std::make_unique<T>(std::forward<Ts>(args)...))
		};
	}

	void tick(double deltaTime) {
		for(const auto& sp: entities) {
			sp->tick(deltaTime);
		}
	}

	void render() {
		auto entityPtrs = allEntities();
		std::ranges::sort(entityPtrs, [](Entity* a, Entity* b) {
			return a->getZOrder() < b->getZOrder();
		});
		for(Entity* e: entityPtrs) {
			e->render();
		}
	}

	void onEvent(const al::Event& ev) {
		auto entityPtrs = allEntities();
		std::ranges::sort(entityPtrs, [](Entity* a, Entity* b) {
			return a->getZOrder() > b->getZOrder();
		});
		for(Entity* e: entityPtrs) {
			if(e->onEvent(ev)) {
				break;
			}
		}
	}


private:

	[[nodiscard]] std::vector<Entity*> allEntities() const {
		std::vector<Entity*> result;
		for(const auto& sp: entities) {
			result.push_back(sp.get());
		}
		return result;
	}

	std::list<std::unique_ptr<Entity>> entities;
};

struct Button: public Entity {
public:
	struct Def {
		al::RectI rect;
		std::string caption;
		std::function<void(void)> callback = [](){};
	} def;

	Button(const Def& def): def(def) {

	}


	enum class State {
		Inactive,
		Hover,
		Down
	};


	void tick(double deltaTime) override {
		static constexpr double half_life = 1.0 / 15.0;
		double w = std::exp2(-deltaTime / half_life);
		currentColor = VecToColor(ColorToVec(currentColor)*w + ColorToVec(targetColor())*(1.0-w));
	}

	void render() override {
		al::TargetBitmap.setClippingRectangle(def.rect);
		al::DrawFilledRectangle(def.rect, currentColor);
		al::Color ccBrightened = VecToColor(ColorToVec(currentColor) + al::Vec3f{0.15, 0.15, 0.15});
		al::Color ccDarkened = VecToColor(ColorToVec(currentColor) - al::Vec3f{0.15, 0.15, 0.15});

		al::DrawLine(def.rect.bottomLeft().asFloat(), def.rect.topLeft().asFloat(), state!=State::Down ? ccBrightened : ccDarkened, 4.0);
		al::DrawLine(def.rect.topLeft().asFloat(), def.rect.topRight().asFloat(), state!=State::Down ? ccBrightened : ccDarkened, 4.0);
		al::DrawLine(def.rect.bottomLeft().asFloat(), def.rect.bottomRight().asFloat(), state!=State::Down ? ccDarkened : ccBrightened, 4.0);
		al::DrawLine(def.rect.bottomRight().asFloat(), def.rect.topRight().asFloat(), state!=State::Down ? ccDarkened : ccBrightened, 4.0);

		al::Vec2i fontOffset(0, -glob::DefaultFont.getLineHeight() / 2);

		DrawShadowedText(def.caption, al::White, def.rect.center() + fontOffset, ALLEGRO_ALIGN_CENTER);

		al::TargetBitmap.resetClippingRectangle();
	}

	bool onEvent(const al::Event& ev) override {
		bool mouseInRect = def.rect.contains(al::Vec2i{ev.mouse.x, ev.mouse.y});
		if(ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
			if(mouseInRect) {
				if(state == State::Inactive) {
					state = State::Hover;
				}
			} else {
				state = State::Inactive;
			}
		} else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			if((ev.mouse.button == al::LMB) && mouseInRect) {
				if(state == State::Hover) {
					state = State::Down;
					def.callback();
					return true;
				}
			}
		} else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
			if((ev.mouse.button == al::LMB) && mouseInRect) {
				if(state == State::Down) {
					state = State::Hover;
				}
			}
		}
		return false;
	}

	int getZOrder() override {
		return 10;
	}

private:

	[[nodiscard]] al::Color targetColor() const {
		switch (state) {
			case State::Inactive: return al::RGB(161, 196, 161);
			case State::Hover: return al::RGB(75, 200, 75);
			case State::Down: return al::RGB(65, 145, 65);
		}
		return al::Black;
	}

	State state = State::Inactive;
	al::Color currentColor = targetColor();
};

struct MessageLog: public Entity {
	std::deque<std::string> messages;
};

struct GraphGridDef {
	al::Vec2f center {};
	SmoothValue<float> zoom {
		.value = 64.0,
		.targetValue = 64.0,
		.halfLife = 1.0 / 15.0
	};

	bool editMode = false;
	std::optional<void*> optNewEdgeSrc = {};

	al::Transform getTransform() {
		return al::Transform::Eye()
			.translate(center)
			.scale({zoom.value, zoom.value})
			.translate(al::CurrentDisplay.size().asFloat() / 2.0);
	}

	al::Vec2f scrToWorld(al::Vec2f scrPos) {
		return getTransform().invert().transform(scrPos);
	}

	al::Vec2f worldToScr(al::Vec2f scrPos) {
		return getTransform().transform(scrPos);
	}
};

struct GraphNode: public Entity {
	int label = 0;
	GraphGridDef* gridDef;
	al::Vec2f center;
	bool hover = false;

	static constexpr float Radius = 0.4;
	static constexpr float OutlineThickness = 0.03;

	struct Circle {
		al::Vec2f center {};
		float radius {};
	};

	struct Effects {
		std::optional<al::Color> overrideColor = {};
		std::optional<std::string> additionalLabel = {};
	} effects = {};

	GraphNode() = default;

	void tick(double deltaTime) override {

	}

	void render() override {

		auto scrCenter = gridDef->worldToScr(this->center);
		auto scrOT = std::max<float>(1.0, OutlineThickness * gridDef->zoom.value);
		auto scrR = std::max<float>(1.0, Radius * gridDef->zoom.value);

		auto defaultColor = effects.overrideColor.value_or(al::White);
		auto hoverColor = al::RGB(190, 200, 255);
		auto editColor = al::RGB(255, 200, 190);
		auto color = defaultColor;
		if(hover) {
			color = gridDef->editMode ? editColor : hoverColor;
		}

		al::DrawFilledCircle(scrCenter, scrR, color);
		al::DrawCircle(scrCenter, scrR, al::Black, scrOT);

		al::Vec2i fontOffset(0, -glob::DefaultFont.getLineHeight() / 2);

		DrawShadowedText(std::to_string(label), al::Black, scrCenter.asInt() + fontOffset, ALLEGRO_ALIGN_CENTER);

		if(effects.additionalLabel) {
			DrawShadowedText(
				effects.additionalLabel.value(),
				al::Blue,
				scrCenter.asInt() - fontOffset,
				ALLEGRO_ALIGN_CENTER
			);
		}
	}

	bool onEvent(const al::Event& ev) override {

		if(ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
			al::Vec2f worldMousePos = gridDef->scrToWorld(al::Vec2f{ev.mouse.x, ev.mouse.y});
			hover = ((this->center - worldMousePos).length() < Radius);
		}

		if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			if(hover) {
				if(ev.mouse.button == al::LMB) {
					if(gridDef->optNewEdgeSrc) {
						glob::WorldEventSource.emitEvent(msg::FinishDrawingVertexEdge {.endVtxLabel = label});
						return true;
					} else if(gridDef->editMode) {
						glob::WorldEventSource.emitEvent(msg::StartDrawingVertexEdge {.beginVtxLabel = label});
						return true;
					}
				} else if(ev.mouse.button == al::RMB) {
					if(gridDef->editMode) {
						glob::WorldEventSource.emitEvent(msg::DeleteVertex {.vtxLabel = label});
						return true;
					}
				}
			}
		}
		return false;
	}

	int getZOrder() override {
		return -7;
	}
};

struct GraphEdge: public Entity {
	int label = -1;
	GraphGridDef* gridDef;
	GraphNode* u;
	GraphNode* v;
	bool hover = false;

	static constexpr float Thickness = 0.05;

	struct Effects {
		std::optional<al::Color> overrideColor = {};
		std::optional<std::string> additionalLabel = {};
	} effects = {};

	GraphEdge() = default;

	void tick(double deltaTime) override {
		
	}


	void render() override {
		auto center1 = gridDef->worldToScr(u->center);
		auto center2 = gridDef->worldToScr(v->center);

		auto thickness = std::max<float>(1.0, Thickness * gridDef->zoom.value);

		if(hover) {
			thickness *= 1.5;
		}

		auto defaultColor = effects.overrideColor.value_or(al::Black);
		auto hoverColor = al::RGB(0, 170, 0);
		auto editColor = al::RGB(170, 0, 0);
		auto color = defaultColor;
		if(hover) {
			color = gridDef->editMode ? editColor : hoverColor;
		}

		al::DrawLine(center1, center2, color, thickness);
		if(effects.additionalLabel) {
			auto middle = (center1 + center2) / 2.0;
			DrawShadowedText(effects.additionalLabel.value(), al::Green, middle.asInt(), ALLEGRO_ALIGN_CENTER);
		}
		
		
	}

	bool onEvent(const al::Event & ev) override {
		
		if(ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
			al::Vec2f mousePos(ev.mouse.x, ev.mouse.y);
			auto worldDist = PointToLineSegmentDist(gridDef->scrToWorld(mousePos), u->center, v->center);
			auto scrDist = worldDist * gridDef->zoom.value;
			hover = (scrDist < std::max<float>(2.0, Thickness*gridDef->zoom.value));
		} else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			if(hover) {
				if(gridDef->editMode && ev.mouse.button == al::RMB) {
					glob::WorldEventSource.emitEvent(msg::DeleteEdge{.edgeLabel = label});
				}
			}
		}

		return false;
	}


	int getZOrder() override {
		return -8;
	}
};

struct GraphGrid: public Entity {
	using Def = GraphGridDef;
	Def def;

	GraphGrid(Def def): def(def) {}

	void updateGraph() {
		if(not graphUpToDate) {
			def.optNewEdgeSrc = std::nullopt;
			std::set<int> vtxsForRemovalSet(verticesForRemoval.begin(), verticesForRemoval.end());
			std::set<int> edgesForRemovalSet(edgesForRemoval.begin(), edgesForRemoval.end());

			for(const auto& e: edgesList) {
				auto u = e.get().u->label;
				auto v = e.get().v->label;
				
				if(vtxsForRemovalSet.contains(u) || vtxsForRemovalSet.contains(v)) {
					edgesForRemovalSet.insert(e.get().label);
				}
			}
			for(auto& v: verticesList) v.get().effects = {};
			for(auto& e: edgesList) e.get().effects = {};

			edgesList.remove_if([&](const World::EntityHandle<GraphEdge>& hEdge){
				return edgesForRemovalSet.contains(hEdge.get().label);
			});
			verticesList.remove_if([&](const World::EntityHandle<GraphNode>& hVtx){
				return vtxsForRemovalSet.contains(hVtx.get().label);
			});

			for(int i=0; auto& hVtx: verticesList) {
				hVtx.get().label = i++;
			}
			for(int i=0; auto& hEdge: edgesList) {
				hEdge.get().label = i++;
			}

			verticesForRemoval.clear();
			edgesForRemoval.clear();
			graphUpToDate = true;
		}
	}

	void tick(double deltaTime) override {

		updateGraph();
		def.editMode = (al::IsKeyDown(ALLEGRO_KEY_LCTRL) || al::IsKeyDown(ALLEGRO_KEY_RCTRL));
		def.zoom.update(deltaTime);

		if(def.editMode) {
			al::CurrentDisplay.setSystemCursor(ALLEGRO_SYSTEM_MOUSE_CURSOR_PRECISION);
		} else {
			al::CurrentDisplay.setSystemCursor(ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);
		}
	}

	void render() override {
		auto worldPos = def.scrToWorld(al::GetMousePos().asFloat());

		DrawText(std::format("({:.3f}, {:.3f})", worldPos.x, worldPos.y), al::Black, {10, 10});

		if(not glob::cfg.displayGraphGrid) {
			return;
		}

		float sw = al::CurrentDisplay.width();
		float sh = al::CurrentDisplay.height();

		auto worldRect = al::RectF{def.scrToWorld({0, 0}), def.scrToWorld({sw, sh})};
		for(float x = floor(worldRect.a.x); x < ceil(worldRect.b.x); x += 1.0f) {
			float sx = def.worldToScr({x, 0.f}).x;
			al::DrawLine({sx, 0.f}, {sx, sh}, al::RGB(224, 224, 224));
		}
		for(float y = floor(worldRect.a.y); y < ceil(worldRect.b.y); y += 1.0f) {
			float sy = def.worldToScr({0.f, y}).y;
			al::DrawLine({0.f, sy}, {sw, sy}, al::RGB(224, 224, 224));
		}

		if(auto* edgeSrc = getOptNewEdgeSrc()) {
			float brightness = std::sin(al::GetTime() * 8.0) * 0.3 + 0.5;
			auto p1 = def.worldToScr(edgeSrc->center);
			auto p2 = al::GetMousePos().asFloat();

			al::DrawLine(p1, p2, al::Gray(brightness), 2.0);
		}
	}


	bool onEvent(const al::Event& ev) override {

		if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			if(ev.mouse.button == al::LMB) {
				if(def.optNewEdgeSrc) {
					def.optNewEdgeSrc = std::nullopt;
					return true;
				}
				if(def.editMode) {
					glob::WorldEventSource.emitEvent(msg::AddVertex {.worldPos = def.scrToWorld(al::GetMousePos().asFloat())});
					return true;
				} else {
					optGrab = GrabData {
						.grabLeftTop = def.center,
						.grabPos = al::GetMousePos().asFloat()
					};
				}
			}
		} else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
			if(ev.mouse.button == al::LMB) {
				optGrab = std::nullopt;
			}
		} else if(ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
			auto mPos = al::GetMousePos().asFloat();
			if(optGrab) {
				def.center = optGrab->grabLeftTop + (mPos - optGrab->grabPos) / def.zoom.value;
			}
			def.zoom.targetValue *= pow(1.2, ev.mouse.dz);
		}

		if(auto optVtxMsg = al::TryGetUserEventData<msg::AddVertex>(ev)) {
			auto hVtx = glob::world->createEntity<GraphNode>();
			auto& v = hVtx.get();
			const auto& msg = optVtxMsg->get();
			v.center = msg.worldPos;
			v.gridDef = &def;

			verticesList.emplace(verticesList.end(), std::move(hVtx));

			graphUpToDate = false;
		} else if(auto optDelVtxMsg = al::TryGetUserEventData<msg::DeleteVertex>(ev)) {
			verticesForRemoval.push_back(optDelVtxMsg->get().vtxLabel);

			graphUpToDate = false;
		} else if(auto optDelEdgeMsg = al::TryGetUserEventData<msg::DeleteEdge>(ev)) {
			edgesForRemoval.push_back(optDelEdgeMsg->get().edgeLabel);

			graphUpToDate = false;
		} else if(auto optStartEdgeMsg = al::TryGetUserEventData<msg::StartDrawingVertexEdge>(ev)) {
			def.optNewEdgeSrc = findGraphNode(optStartEdgeMsg->get().beginVtxLabel);
		} else if(auto optFinEdgeMsg = al::TryGetUserEventData<msg::FinishDrawingVertexEdge>(ev)) {
			
			if(auto* srcEdge = getOptNewEdgeSrc()) {
				auto hEdge = glob::world->createEntity<GraphEdge>();
				auto& e = hEdge.get();
				const auto& msg = optFinEdgeMsg->get();
				auto* dstEdge = findGraphNode(optFinEdgeMsg->get().endVtxLabel);

				e.gridDef = &def;
				e.u = srcEdge;
				e.v = dstEdge;

				edgesList.emplace(edgesList.end(), std::move(hEdge));
				graphUpToDate = false;
			}
			

		} else {
			return false;
		}

		return true;
	}

	int getZOrder() override {
		return -10;
	}

	struct LabeledGraph {
		g2x::static_simple_graph graph;
		std::vector<GraphNode*> nodeMap;
		std::vector<GraphEdge*> edgeMap;
	};
	
	LabeledGraph get_graph() {
		updateGraph();
		
		int numVertices = verticesList.size();
		std::vector<std::pair<int, int>> ctorEdges;
		std::map<int, GraphNode*> nodeMap;
		std::map<std::pair<int,int>, GraphEdge*> edgeMap;
		for(auto& hEdge: edgesList) {
			int u = hEdge.get().u->label;
			int v = hEdge.get().v->label;
			ctorEdges.emplace_back(u, v);
			edgeMap[{u, v}] = &hEdge.get();
			printf("u,v %d,%d ---> %p\n",u,v,(void*)edgeMap[{u,v}]);
		}
		for(auto& hVtx: verticesList) {
			int v = hVtx.get().label;
			nodeMap[v] = &hVtx.get();
		}
		g2x::static_simple_graph graph {numVertices, ctorEdges};


		auto nodes = g2x::create_vertex_label_container(graph, (GraphNode*)nullptr);
		auto edges = g2x::create_edge_label_container(graph, (GraphEdge*)nullptr);

		for(const auto& v: g2x::all_vertices(graph)) {
			nodes[v] = nodeMap[v];
		}
		for(const auto& [u, v, i]: g2x::all_edges(graph)) {
			edges[i] = edgeMap[{u, v}];
			if(not edges[i]) {
				edges[i] = edgeMap[{v, u}];
			}
			printf("u,v,i %d,%d,%d -> %p\n",u,v,i,(void*)edges[i]);
		}

		return LabeledGraph {std::move(graph), nodes, edges};
	}

	void bfsFromZero() {
		const auto& [graph, nodeMap, edgeMap] = get_graph();
		if(g2x::num_vertices(graph) == 0) return;

		auto distances = g2x::create_vertex_label_container(graph, -1);

		for(const auto& [u, v, i]: g2x::algo::simple_edges_bfs(graph, 0)) {
			if(distances[u] == -1) {
				distances[u] = 0;
			}
			distances[v] = distances[u] + 1;

			edgeMap[i]->effects.overrideColor = al::LightMagenta;
		}

		for(const auto& v: g2x::all_vertices(graph)) {
			nodeMap[v]->effects.additionalLabel = std::format("d={}", distances[v]);
		}


	}

	void dfsFromZero() {
		const auto& [graph, nodeMap, edgeMap] = get_graph();
		if(g2x::num_vertices(graph) == 0) return;

		auto sources = g2x::create_vertex_label_container(graph, -1);
		for(const auto& [u, v, i]: g2x::algo::simple_edges_dfs(graph, 0)) {
			sources[v] = u;
		}

		for(const auto& v: g2x::all_vertices(graph)) {
			nodeMap[v]->effects.additionalLabel = std::format("src={}", sources[v]);
		}

	}

	void maxBipartiteMatching() {
		const auto& [graph, nodeMap, edgeMap] = get_graph();
		if(g2x::num_vertices(graph) == 0) return;

		try {
			auto matching = g2x::algo::max_bipartite_matching(graph);

			for(const auto& [u, v, i]: g2x::all_edges(graph)) {
				if(matching[i]) {
					edgeMap[i]->effects.overrideColor = al::LightRed;
				}
			}
		} catch (std::exception&) {
			al::MessageBox("Error", "", "Odd cycle detected - graph is not bipartite, aborting");
		}


	}

	void cycleCover() {

		al::MessageBox("Not implemented", "Not implemented");

	}



private:
	bool graphUpToDate = false;
	std::vector<int> verticesForRemoval;
	std::vector<int> edgesForRemoval;
	std::list<World::EntityHandle<GraphNode>> verticesList;
	std::list<World::EntityHandle<GraphEdge>> edgesList;

	GraphNode* getOptNewEdgeSrc() {
		return static_cast<GraphNode*>(def.optNewEdgeSrc.value_or(nullptr));
	}

	GraphNode* findGraphNode(int index) {
		for(auto& v: verticesList) {
			if(v.get().label == index) {
				return &v.get();
			}
		}
		return nullptr;
	}

	struct GrabData {
		al::Vec2f grabLeftTop;
		al::Vec2f grabPos;
	};
	std::optional<GrabData> optGrab;

	

};



int main() {

	al::Display disp(800, 600, ALLEGRO_RESIZABLE);

	al::EventLoop loop(al::DemoEventLoopConfig);
	World world;
	glob::world = &world;
	try {
		glob::DefaultFont = al::Font("resources/roboto.ttf", 16);
	} catch (al::ResourceLoadError& ex) {
		glob::DefaultFont = al::Font::CreateBuiltinFont();
	}

	auto hGraphGrid = world.createEntity<GraphGrid>(GraphGridDef {});

	auto hBtn1 = world.createEntity<Button>(Button::Def{
		.rect = al::RectI::XYWH(30, 30, 200, 25),
		.caption = "Export graph to .txt",
		.callback = [&]() {
		}
	});
	auto hBtn2 = world.createEntity<Button>(Button::Def{
		.rect = al::RectI::XYWH(30, 70, 200, 25),
		.caption = "BFS from 0",
		.callback = [&]() {
			hGraphGrid.get().bfsFromZero();
		}
	});
	auto hBtn3 = world.createEntity<Button>(Button::Def{
		.rect = al::RectI::XYWH(30, 110, 200, 25),
		.caption = "DFS from 0",
		.callback = [&]() {
			hGraphGrid.get().dfsFromZero();
		}
	});
	auto hBtn4 = world.createEntity<Button>(Button::Def{
		.rect = al::RectI::XYWH(30, 150, 200, 25),
		.caption = "Max bipartite matching",
		.callback = [&]() {
			hGraphGrid.get().maxBipartiteMatching();
			
		}
	});
	auto hBtn5 = world.createEntity<Button>(Button::Def{
		.rect = al::RectI::XYWH(30, 190, 200, 25),
		.caption = "Cycle cover",
		.callback = [&]() {
			hGraphGrid.get().cycleCover();
		}
	});


	loop.eventQueue.registerSource(glob::WorldEventSource);
	loop.eventDispatcher.setCatchallHandler([&](const al::Event& ev) {
		world.onEvent(ev);
		return al::EventHandled;
	});
	loop.framerateLimiter.setLimit(al::FPSLimit::None);
	loop.run([&] {
		al::TargetBitmap.clearToColor(al::White);

		world.tick(loop.getLastTickTime());
		world.render();
		DrawText(std::format("{} fps", loop.getFPS()), al::Black, {al::CurrentDisplay.width()-10, 10}, ALLEGRO_ALIGN_RIGHT);
		DrawText("LMB - drag view", al::Black, {al::CurrentDisplay.width()-10, 30}, ALLEGRO_ALIGN_RIGHT);
		DrawText("Mouse wheel - zoom in/out", al::Black, {al::CurrentDisplay.width()-10, 50}, ALLEGRO_ALIGN_RIGHT);
		DrawText("Ctrl+LMB - insert node or edge", al::Black, {al::CurrentDisplay.width()-10, 70}, ALLEGRO_ALIGN_RIGHT);
		DrawText("Ctrl+RMB - delete node or edge", al::Black, {al::CurrentDisplay.width()-10, 90}, ALLEGRO_ALIGN_RIGHT);


		al::CurrentDisplay.flip();
	});
}