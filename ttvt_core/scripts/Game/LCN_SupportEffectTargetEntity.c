[EntityEditorProps(category: "LCN/Support", description: "Visible marker used as an optional target position for LCN support consoles", color: "64 176 255 255", color2: "64 176 255 48", visible: true, style: "sphere", dynamicBox: true)]
class LCN_SupportEffectTargetEntityClass : GenericEntityClass
{
}

class LCN_SupportEffectTargetEntity : GenericEntity
{
	[Attribute("5", UIWidgets.EditBox, "Workbench marker radius in meters", params: "1 100 1", category: "LCN Support")]
	protected float m_fMarkerRadius;

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	override void _WB_GetBoundBox(inout vector min, inout vector max, IEntitySource src)
	{
		float radius = Math.Max(m_fMarkerRadius, 1.0);
		min = Vector(-radius, -1, -radius);
		max = Vector(radius, 1, radius);
	}
#endif
}
